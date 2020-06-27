#include <backend/cogen.hpp>
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

  gccjit::context ctx;
};

gccjit::type cogen_int(generator& gen, Constructor::cRef ctor, Node::cRef arg)
{
  // TODO: emit normal error?
  assert(arg->kind() == NodeKind::Literal && "int is expected to have a constant value as argument here.");
  Literal::cRef l = arg->to<Literal>();

  const bool is_unsigned = ctor->name == symbol("u");
  switch(l->literal)
  {
  case 8:  return gen.ctx.get_type(is_unsigned ? GCC_JIT_TYPE_UNSIGNED_CHAR  : GCC_JIT_TYPE_CHAR);
  case 16: return gen.ctx.get_type(is_unsigned ? GCC_JIT_TYPE_UNSIGNED_SHORT : GCC_JIT_TYPE_SHORT);
  case 32: return gen.ctx.get_type(is_unsigned ? GCC_JIT_TYPE_UNSIGNED_INT   : GCC_JIT_TYPE_INT);
  }
  assert(false && "unsupported int size for this backend.");
  return *((gccjit::type*)nullptr);
}

void cogen(generator& gen, const Node* ref)
{
  //////////////////////////////// TYPES
  auto genty = [&gen](auto genty, Node::cRef node) -> gccjit::type
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
          case hash_string("i"): return cogen_int(gen, c, ap->arg());

          // Pointer
          case hash_string("_Ptr"): {
              return genty(genty, ap->arg()).get_pointer();
            } break;
          }
        }
      } break;
    }
    assert(false && "Unsupported type");
    return *((gccjit::type*)nullptr);
  };
  //////////////////////////////// COGEN
  //NodeMap<gccjit::block> fns;
  std::unordered_map<Node::cRef, gccjit::block, NodeHasher, NodeComparator> fns;
  NodeMap<gccjit::rvalue> rvals;
  NodeMap<gccjit::lvalue> lvals;

  std::optional<Fn::cRef> external_fn;
  auto cogen = [&gen,&fns,&lvals,&rvals,&genty,&external_fn](auto cogen, const Node* node, gccjit::block* cur_block) -> void
  {
    switch(node->kind())
    {
    case NodeKind::Param: {
        assert(rvals.contains(node) && "Param needs to be bound before it can be used.");
        return ;
      } break;

    case NodeKind::Fn: {
      Fn::cRef fn = node->to<Fn>();
      // check if function already exists and can be used for calling
      if(auto it = fns.find(fn); it != fns.end())
        return ;

      if(fn->is_external())
      {
        assert(cur_block == nullptr && "cur_block must be null for external functions");

        // generate ordinary function
        std::vector<gccjit::param> params;
        if(fn->arg()->kind() == NodeKind::Tup)
        {
          // If we have a function such as `main(argc : int, argv : const char**)` we generate more than one param
          Tup::cRef tup = fn->arg()->to<Tup>();
          for(std::size_t i = 0; i < tup->argc(); ++i)
          {
            auto par = gen.ctx.new_param(genty(genty, tup->type()->me()[i]), tup->me()[i]->unique_name().get_string().c_str());

            rvals[tup->me()[i]] = par;
            params.push_back(par);
          }
        }
        else
        {
          auto par = gen.ctx.new_param(genty(genty, fn->arg()->type()), fn->arg()->unique_name().get_string().c_str());
          rvals[fn->arg()] = par; // <- one should not be able to change parameter values in an external function, thus the missing lvals insertion

          params.push_back(par);
        }
        assert(fn->ret() != nullptr && "ret continuation must not be null.");

        auto jit_fn = gen.ctx.new_function(fn->is_external() ? GCC_JIT_FUNCTION_EXPORTED : GCC_JIT_FUNCTION_IMPORTED,
                                           genty(genty, fn->ret()->to<Fn>()->arg()->type()),
                                           (fn->is_external() ? fn->external_name() : fn->unique_name()).get_string(),
                                           params, 0);
        fns[fn] = jit_fn.new_block(fn->unique_name().get_string().c_str());

        external_fn = fn;
        cogen(cogen, fn->bdy(), &fns[fn]);
        external_fn = std::nullopt;
      }
      else
      {
        assert(cur_block != nullptr && "cur_block must not be null for internal functions.");

        // Generate basic block within a function
        if(fn->arg()->kind() == NodeKind::Tup)
        {
          // If we have a function such as `foo(n : int, m : int)` we generate more than one local
          auto tup = *fn->arg()->to<Tup>();
          for(std::size_t i = 0; i < tup.argc(); ++i)
          {
            rvals[tup[i]] = lvals[tup[i]] = cur_block->get_function()
                            .new_local(genty(genty, tup[i]),
                                       tup[i]->unique_name().get_string().c_str());
          }
        }
        else
        {
          rvals[fn->arg()] = lvals[fn->arg()] = cur_block->get_function()
            .new_local(genty(genty, fn->arg()->type()),
                       fn->arg()->unique_name().get_string().c_str());
        }
        auto new_block = fns[fn] = cur_block->get_function().new_block(fn->unique_name().get_string().c_str());

        // go from old block to new block. This is the same as a curried function call
        cogen(cogen, fn->bdy(), &new_block);

        //cur_block->end_with_jump(new_block);
      }
      return ;
    } break;

    case NodeKind::App: {
      App::cRef ap = node->to<App>();

      const bool is_external = ap->caller() == external_fn.value()->ret();
      if(!is_external)
        cogen(cogen, ap->caller(), cur_block);
      cogen(cogen, ap->arg(), cur_block);

      // TODO: what if caller is a higher order function? We need to subst!
      if(is_external)
      {
        // external_ret must adhere the calling convention by just literally returning a value
        if(ap->arg()->kind() == NodeKind::Unit)
          cur_block->end_with_return(); // <- function returns void
        else
        {
          rvals[ap->arg()] = gen.ctx.new_cast(rvals[ap->arg()], genty(genty, ap->caller()->to<Fn>()->arg()->type()));
          cur_block->end_with_return(rvals[ap->arg()]);
        }
      }
      else
      {
        // Not an external function call, we need to set the parameters and jump to the caller
        cur_block->add_assignment(lvals[ap->caller()->to<Fn>()->arg()], rvals[ap->arg()]);
        cur_block->end_with_jump(fns[ap->caller()]);
      }
      return ;
    } break;

    case NodeKind::Literal: {
      rvals[node] = gen.ctx.new_rvalue(gen.ctx.get_type(GCC_JIT_TYPE_LONG_LONG),
                                       static_cast<long int>(node->to<Literal>()->literal));
      return ;
    } break;

    case NodeKind::Case: {
      Case::cRef cs = node->to<Case>();

      cogen(cogen, cs->of(), cur_block);
      assert(rvals.contains(cs->of()) && "thing we match on should've been generated already");
      auto ofty = genty(genty, cs->of()->type());

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
        cogen(cogen, m.first, &cond);
        assert(rvals.contains(m.first) && "rvals should contain the pattern by now");

        // cs->of() == pattern   -T-> tr_b    -F-> fl_b
        auto eq = gen.ctx.new_eq(rvals[cs->of()], gen.ctx.new_cast(rvals[m.first], ofty));
        cond.end_with_conditional(eq, tr_b, fl_b);
        
        // Generate pattern expression thing inside the tr_b branch
        cogen(cogen, m.second, &tr_b);
        tr_b.add_assignment(cs_v, gen.ctx.new_cast(rvals[m.second], ofty));

        // join the true and false branches
        tr_b.end_with_jump(join);

        // tie the knot
        cur = fl_b;
      }
      cur.end_with_jump(join);
      // update the block one layer up.
      *cur_block = join;
      return ;
    } break;

    case NodeKind::Binary: {
      Binary::cRef bn = node->to<Binary>();

      cogen(cogen, bn->lhs(), cur_block);
      cogen(cogen, bn->rhs(), cur_block);
      // results are stored in rvals by now
      assert(rvals.count(bn->lhs()) && rvals.count(bn->rhs()) && "Bug in codegen.");

      auto lty = genty(genty, bn->lhs()->type());
      auto l = gen.ctx.new_cast(rvals[bn->lhs()], lty);
      auto r = gen.ctx.new_cast(rvals[bn->rhs()], lty);
      switch(bn->op)
      {
      case BinaryKind::Minus: rvals[bn] = gen.ctx.new_binary_op(GCC_JIT_BINARY_OP_MINUS, lty, l, r); break;
      case BinaryKind::Plus:  rvals[bn] = gen.ctx.new_binary_op(GCC_JIT_BINARY_OP_PLUS,  lty, l, r); break;
      case BinaryKind::Mult:  rvals[bn] = gen.ctx.new_binary_op(GCC_JIT_BINARY_OP_MULT, lty, l, r);  break;
      }
      return ;
    } break;
    }
    assert(false && "Unsupported node for this backend.");
  };
  cogen(cogen, ref, nullptr);
}

bool cogen(std::string name, const Node* ref)
{
  generator gen(name);
  cogen(gen, ref);

  // always emit assembler for now, is most versatile
  gen.ctx.compile_to_file(GCC_JIT_OUTPUT_KIND_EXECUTABLE, (name + ".out").c_str());
  return true;
}

}


