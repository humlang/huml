#include <backend/cogen.hpp>
#include <backend/builder.hpp>
#include <backend/node.hpp>
#include <tmp.hpp>

#include <libgccjit++.h>

namespace ir
{

std::vector<Node::cRef> find_reachable_fns(Node::cRef foo)
{
  std::vector<Node::cRef> v;
  if(foo->kind() == NodeKind::Fn)
    v.insert(v.end(), foo);
  for(std::size_t i = 0; i < foo->argc(); ++i)
  {
    auto w = find_reachable_fns(foo->me()[i]);
    v.insert(v.end(), w.begin(), w.end());
  }
  return v;
}

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
    case NodeKind::Tup: {
      assert(false && "Unimplemented!");
    } break;
    case NodeKind::Fn: {
      assert(false && "Function pointers are not supported currently.");
    } break;
    }
    assert(false && "Unsupported type");
    return *((gccjit::type*)nullptr);
  };

  //////////////////////////////// COGEN
  void cogen(const Node* node)
  {
    assert(node->kind() == NodeKind::Fn && node->to<Fn>()->is_external() && "function to generate from must be external");

    /// Find return continuation
    Node::cRef ret = nullptr;
    for(auto& v : node->to<Fn>()->uncurry())
    {
      if(v->type()->kind() == NodeKind::Fn)
        ret = v;
    }
    // TODO: this assumes that the continuation is not hidden under a tuple. Should be ok if we enforce this somehow!
    assert(ret != nullptr && ret->type()->kind() == NodeKind::Fn && "return continuation must have a function type");

    /// Build function
    assert(ret->to<Fn>()->args().size() == 1 && "We only support one single return type atm");
    auto ret_type = genty(ret->to<Fn>()->args().front()->type());
    // TODO: check return type and potentially return a tuple
    
    std::vector<gccjit::param> params;
    for(auto& v : node->to<Fn>()->uncurry())
    {
      if(v->type()->kind() == NodeKind::Unit || v->type()->kind() == NodeKind::Fn)
        continue;
      // TODO: potentially emit tuple type

      auto typ = genty(v->type());
      auto param = ctx.new_param(typ, v->unique_name().get_string().c_str());

      lvals.emplace(v, std::vector<gccjit::lvalue> { param });
      rvals.emplace(v, std::vector<gccjit::rvalue> { param });

      params.emplace_back(param);
    }
    gccjit::function gccfn = ctx.new_function(GCC_JIT_FUNCTION_EXPORTED, ret_type,
                                              (node->to<Fn>()->is_external()
                                               ? node->to<Fn>()->external_name() : node->unique_name()).get_string().c_str(),
                                              params, false);
    // TODO: do we need a particular schedule?
    std::vector<Node::cRef> rfns = find_reachable_fns(node->to<Fn>());

    /// emit *all* (reachable) parameters and function blocks
    for(auto& x : rfns)
    {
      if(x == ret || fns.contains(x))
        continue;

      for(auto& a : x->to<Fn>()->args())
      {
        if(a->type()->kind() == NodeKind::Unit || a->type()->kind() == NodeKind::Fn)
          continue;

        // TODO: emit tuple type
        auto a_ = gccfn.new_local(genty(a->type()), a->unique_name().get_string().c_str());
        auto pa = gccfn.new_local(genty(a->type()), ("p" + a->unique_name().get_string()).c_str());

        lvals.emplace(a, std::vector<gccjit::lvalue> { a_, pa });
        rvals.emplace(a, std::vector<gccjit::rvalue> { a_, pa });
      }
      fns[x] = gccfn.new_block(("b" + x->unique_name().get_string()).c_str());
    }

    /// emit all cfnodes
    for(auto& x : rfns)
    {
      auto v = x->to<Fn>();
      if(v == ret)
        continue;

      assert(fns.contains(v) && "Basicblock of fn must have been created");
      gccjit::block blk = fns[v];

      // load param from phi
      for(auto& a : v->args())
      {
        if(a->type()->kind() != NodeKind::Unit && a->type()->kind() != NodeKind::Fn)
          blk.add_assignment(lvals[a].front(), rvals[a].back());
      }

      // close lam
      if(v->bdy()->kind() == NodeKind::Case)
      {
        assert(false && "Unimplemented");
      }
      else if(v->bdy()->kind() == NodeKind::App)
      {
        cogen_handle_app(v->bdy()->to<App>(), &blk, ret->to<Fn>());
      }
    }
  }
  void cogen_handle_app(App::cRef v, gccjit::block* blk, Fn::cRef ret)
  {
    if(v->caller() == ret)
    {
      // just simply return
      if(v->arg()->type()->kind() == NodeKind::Unit)
        blk->end_with_return();
      else
      {
        auto vret = ret->uncurry();
        assert(vret.size() == 1 && "We don't support returning tuples right now."); // TODO: tuples...

        blk->end_with_return(cogen(v->arg(), blk));
      }
    }
    else
    {
      // If this is a basic block, we can simply jump
      if(v->caller()->kind() == NodeKind::Fn)
      {
        auto x = v->caller()->to<Fn>();
        for(auto& a : x->args())
        {
          assert(lvals.contains(a) && rvals.contains(a) && "App args must exist in fn call.");

          blk->add_assignment(lvals[a].back(), rvals[v->arg()].front());
        }
        assert(fns.contains(v->caller()) && "Caller must be contained in the set.");
        blk->end_with_jump(fns[v->caller()]);
      }
      else
      {
        // TODO
        assert(false && "Ordinary function calls are unimplemented");
      }
    }
  }
  gccjit::rvalue cogen(const Node* node, gccjit::block* blk)
  {
    if(auto it = rvals.find(node); it != rvals.end())
      return it->second.front();

    if(node->kind() == NodeKind::Binary)
    {
      auto bin = node->to<Binary>();

      auto lhs_typ = genty(bin->lhs()->type());

      auto lhs = ctx.new_cast(cogen(bin->lhs(), blk), lhs_typ);
      auto rhs = ctx.new_cast(cogen(bin->rhs(), blk), lhs_typ);
      switch(bin->op)
      {
      case BinaryKind::Minus: return ctx.new_minus(lhs_typ, lhs, rhs);
      case BinaryKind::Plus:  return ctx.new_plus(lhs_typ, lhs, rhs);
      case BinaryKind::Mult:  return ctx.new_mult(lhs_typ, lhs, rhs);
      }
    }

    if(node->kind() == NodeKind::Literal)
    {
      return ctx.new_rvalue(ctx.get_type(GCC_JIT_TYPE_LONG), static_cast<long>(node->to<Literal>()->literal));
    }

    if(node->kind() == NodeKind::Case)
    {
      // TODO: implement
    }

    assert(false && "unreachable");
    return {};
  }
public:
  gccjit::context ctx;

private:
  NodeMap<gccjit::block> fns;
  //std::unordered_map<Node::cRef, gccjit::block, NodeHasher, NodeComparator> fns;
  NodeMap<std::vector<gccjit::rvalue>> rvals;
  NodeMap<std::vector<gccjit::lvalue>> lvals;

  std::optional<Fn::cRef> external_fn;
};

bool cogen(std::string name, const Node* ref)
{
  generator gen(name);
  gen.cogen(ref);

  // always emit assembler for now, is most versatile
  gen.ctx.compile_to_file(GCC_JIT_OUTPUT_KIND_EXECUTABLE, (name + ".out").c_str());
  return true;
}

}


