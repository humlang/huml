#include <backend/cogen.hpp>
#include <backend/builder.hpp>
#include <backend/defunc.hpp>
#include <backend/node.hpp>
#include <tmp.hpp>

#include <libgccjit++.h>

namespace ir
{

struct generator
{
  generator(std::string name)
    : ctx(gccjit::context::acquire())
  {
    ctx.set_int_option(GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, 2);
  }

  ~generator()
  {
    ctx.release();
  }

  //////////////////////////////// TYPES
  gccjit::type cogen_int(Constructor::cRef ctor, Node::cRef arg)
  {
    // TODO: emit normal error?
    assert(arg->kind() == NodeKind::Literal && "int is expected to have a constant value as argument here.");
    Literal::cRef l = arg->to<Literal>();

    const bool is_unsigned = ctor->name == symbol("u");
    switch(l->literal)
    {
    case 8:  return ctx.get_type(is_unsigned ? GCC_JIT_TYPE_UNSIGNED_CHAR  : GCC_JIT_TYPE_CHAR);
    case 16: return ctx.get_type(is_unsigned ? GCC_JIT_TYPE_UNSIGNED_SHORT : GCC_JIT_TYPE_SHORT);
    case 32: return ctx.get_type(is_unsigned ? GCC_JIT_TYPE_UNSIGNED_INT   : GCC_JIT_TYPE_INT);
    }
    assert(false && "unsupported int size for this backend.");
    return *((gccjit::type*)nullptr);
  }
  gccjit::type genty(Node::cRef node)
  {
    switch(node->kind())
    {
    case NodeKind::App: {
      App::cRef ap = node->to<App>();

      if(ap->caller()->kind() == NodeKind::Ctr)
      {
        Constructor::cRef c = ap->caller()->to<Constructor>();
        switch(c->name.get_hash())
        {
        // Int, Char
        case hash_string("u"):
        case hash_string("i"): return cogen_int(c, ap->arg());

        // Pointer
        case hash_string("_Ptr"): {
            return genty(ap->arg()).get_pointer();
          } break;
        }
      }
    } break;
    case NodeKind::Fn: {
      Fn::cRef fn = node->to<Fn>();
      auto arg = genty(fn->arg()->type()).get_inner_type();
      auto ret = genty(fn->bdy()).get_inner_type();

      // TODO: Make this work for tuples
      return gcc_jit_context_new_function_ptr_type(ctx.get_inner_context(), NULL, ret, 1, &arg, 0);
    } break;
    }
    assert(false && "Unsupported type");
    return *((gccjit::type*)nullptr);
  };

  //////////////////////////////// COGEN
  void cogen(const Node* node, gccjit::block* cur_block)
  {
#if 0
    switch(node->kind())
    {
    case NodeKind::Param: {
        assert(rvals.contains(node) && "Param needs to be bound before it can be used.");
        return ;
      } break;

    case NodeKind::Fn:  gen_fn(node->to<Fn>(), cur_block); return ;
    case NodeKind::App: gen_app(node->to<App>(), cur_block); return ;

    case NodeKind::Literal: {
      rvals[node] = ctx.new_rvalue(ctx.get_type(GCC_JIT_TYPE_LONG_LONG),
                                   static_cast<long int>(node->to<Literal>()->literal));
      return ;
    } break;

    case NodeKind::Case:   gen_case(node->to<Case>(), cur_block); return ;
    case NodeKind::Binary: gen_binary(node->to<Binary>(), cur_block); return ;
    }
    assert(false && "Unsupported node for this backend.");
  };
private:
  void gen_fn(Fn::cRef fn, gccjit::block* cur_block)
  {
    // check if function already exists and can be used for calling
    if(auto it = fns.find(fn); it != fns.end())
      return ;
    // generate ordinary function
    std::vector<gccjit::param> params;
    if(fn->arg()->kind() == NodeKind::Tup)
    {
      // If we have a function such as `main(argc : int, argv : const char**)` we generate more than one param
      Tup::cRef tup = fn->arg()->to<Tup>();
      params.reserve(tup->argc());
      for(std::size_t i = 0; i < tup->argc(); ++i)
      {
        auto par = ctx.new_param(genty(tup->type()->me()[i]), tup->me()[i]->unique_name().get_string().c_str());

        rvals[tup->me()[i]] = par;
        params.push_back(par);
      }
    }
    else
    {
      auto par = ctx.new_param(genty(fn->arg()->type()), fn->arg()->unique_name().get_string().c_str());
      if(!fn->is_external())
        lvals[fn->arg()] = par; // <- One should not be able to change the param of external values
      rvals[fn->arg()] = par;

      params.push_back(par);
    }
    assert(fn->ret() != nullptr && "ret continuation must not be null.");
    auto jit_fn = fn->is_external()
                  ? ctx.new_function(GCC_JIT_FUNCTION_EXPORTED,
                                     genty(fn->ret()->to<Fn>()->arg()->type()),
                                     fn->external_name().get_string(),
                                     params, 0)
                  : ctx.new_function(GCC_JIT_FUNCTION_INTERNAL,
                                     ctx.get_type(GCC_JIT_TYPE_VOID), // All functions we generate return simply void, because they are continuations!
                                     fn->unique_name().get_string(),
                                     params, 0);
    rvals[fn] = jit_fn.get_address();
    fns[fn] = jit_fn.new_block(fn->unique_name().get_string().c_str());

    if(fn->is_external())
      external_fn = fn;

    // Gen body
    cogen(fn->bdy(), &fns[fn]);

    if(fn->is_external())
      external_fn = std::nullopt;
  }
  void gen_app(App::cRef ap, gccjit::block* cur_block)
  {
    // TODO: Make use of defunctionalization pass to optimize this madness!
    
    const bool is_external_ret = ap->caller() == external_fn.value()->ret();
    if(!is_external_ret)
      cogen(ap->caller(), cur_block);
    cogen(ap->arg(), cur_block);

    // only emit a function call if the function we call is not an external ret
    if(!is_external_ret)
    {
      auto call = ctx.new_call(fns[ap->caller()].get_function(), rvals[ap->arg()]);

      fns[ap->caller()].add_eval(rvals[ap] = call);
    }

    if(ap->arg()->kind() == NodeKind::Unit || !is_external_ret)
      cur_block->end_with_return(); // <- function returns void if we call with () or if it is not external
    else
    {
      rvals[ap->arg()] = ctx.new_cast(rvals[ap->arg()], genty(ap->caller()->to<Fn>()->arg()->type()));
      cur_block->end_with_return(rvals[ap->arg()]);
    }
  }
  void gen_case(Case::cRef cs, gccjit::block* cur_block)
  {
    cogen(cs->of(), cur_block);
    assert(rvals.contains(cs->of()) && "thing we match on should've been generated already");
    auto ofty = genty(cs->of()->type());

    // We need to somehow store the value
    auto cs_v = cur_block->get_function().new_local(ofty, cs->unique_name().get_string().c_str());
    rvals[cs] = lvals[cs] = cs_v;

    auto fn = cur_block->get_function();
    auto ma = cs->match_arms();
    gccjit::block cur = *cur_block;
    auto join = fn.new_block(("join" + cs->unique_name().get_string()).c_str());
    for(auto& m : ma)
    {
      auto cond = fn.new_block(("cond" + m.first->unique_name().get_string()).c_str());
      cur.end_with_jump(cond);

      auto tr_b = fn.new_block(("tr_b" + m.second->unique_name().get_string()).c_str());
      auto fl_b = fn.new_block(("fl_b" + m.first->unique_name().get_string()).c_str());

      // TODO: This does not work well for `_` or more advanced patterns such as `\_. _ + e`
      cogen(m.first, &cond);
      assert(rvals.contains(m.first) && "rvals should contain the pattern by now");

      // cs->of() == pattern   -T-> tr_b    -F-> fl_b
      auto eq = ctx.new_eq(rvals[cs->of()], ctx.new_cast(rvals[m.first], ofty));
      cond.end_with_conditional(eq, tr_b, fl_b);
      
      // Generate pattern expression thing inside the tr_b branch
      cogen(m.second, &tr_b);
      tr_b.add_assignment(cs_v, ctx.new_cast(rvals[m.second], ofty));

      // join the true and false branches
      tr_b.end_with_jump(join);

      // tie the knot
      cur = fl_b;
    }
    cur.end_with_jump(join);
    // update the block one layer up.
    *cur_block = join;
  }
  void gen_binary(Binary::cRef bn, gccjit::block* cur_block)
  {
    cogen(bn->lhs(), cur_block);
    cogen(bn->rhs(), cur_block);
    // results are stored in rvals by now
    assert(rvals.count(bn->lhs()) && rvals.count(bn->rhs()) && "Bug in codegen.");

    auto lty = genty(bn->lhs()->type());
    auto l = ctx.new_cast(rvals[bn->lhs()], lty);
    auto r = ctx.new_cast(rvals[bn->rhs()], lty);
    switch(bn->op)
    {
    case BinaryKind::Minus: rvals[bn] = ctx.new_binary_op(GCC_JIT_BINARY_OP_MINUS, lty, l, r); break;
    case BinaryKind::Plus:  rvals[bn] = ctx.new_binary_op(GCC_JIT_BINARY_OP_PLUS,  lty, l, r); break;
    case BinaryKind::Mult:  rvals[bn] = ctx.new_binary_op(GCC_JIT_BINARY_OP_MULT, lty, l, r);  break;
    }
#endif
  }
public:
  gccjit::context ctx;

private:
  //NodeMap<gccjit::block> fns;
  std::unordered_map<Node::cRef, gccjit::block, NodeHasher, NodeComparator> fns;
  NodeMap<gccjit::rvalue> rvals;
  NodeMap<gccjit::lvalue> lvals;

  std::optional<Fn::cRef> external_fn;
};

bool cogen(std::string name, const Node* ref)
{
  builder cpy;
  ref = defunctionalize(cpy, ref);

  generator gen(name);
  //gen.cogen(ref, nullptr);

  // always emit assembler for now, is most versatile
  gen.ctx.compile_to_file(GCC_JIT_OUTPUT_KIND_EXECUTABLE, (name + ".out").c_str());
  return true;
}

}


