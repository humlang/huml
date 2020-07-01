#include <backend/cogen.hpp>
#include <backend/builder.hpp>
#include <backend/node.hpp>
#include <tmp.hpp>

#include <libgccjit++.h>

namespace ir
{

NodeSet find_reachable_fns(Node::cRef foo)
{
  NodeSet v;
  if(foo->kind() == NodeKind::Fn)
    v.insert(foo);
  for(std::size_t i = 0; i < foo->argc(); ++i)
  {
    auto w = find_reachable_fns(foo->me()[i]);
    v.insert(w.begin(), w.end());
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
    auto ret_type = genty(ret->to<Fn>()->arg());
    // TODO: check return type and potentially return a tuple
    
    std::vector<gccjit::param> params;
    for(auto& v : node->to<Fn>()->uncurry())
    {
      if(v->type()->kind() == NodeKind::Unit || v->type()->kind() == NodeKind::Fn)
        continue;
      // TODO: potentially emit tuple type

      auto typ = genty(v);
      auto param = ctx.new_param(typ, v->unique_name().get_string().c_str());

      lvals.emplace(v, std::vector<gccjit::lvalue> { param });
      rvals.emplace(v, std::vector<gccjit::rvalue> { param });

      params.emplace_back(param);
    }
    gccjit::function gccfn = ctx.new_function(GCC_JIT_FUNCTION_EXPORTED, ret_type,
                                              node->unique_name().get_string().c_str(),
                                              params, false);
    // TODO: do we need a particular schedule?
    NodeSet rfns = find_reachable_fns(node->to<Fn>());

    /// emit *all* parameters
    for(auto& x : rfns)
    {
      auto v = x->to<Fn>();
      if(v->arg()->type()->kind() == NodeKind::Unit)
        continue;
      // TODO: emit tuple type

      auto a  = gccfn.new_local(genty(v), v->unique_name().get_string().c_str());
      auto pa = gccfn.new_local(genty(v), ("p" + v->unique_name().get_string()).c_str());

      lvals.emplace(v, std::vector<gccjit::lvalue> { a, pa });
      rvals.emplace(v, std::vector<gccjit::rvalue> { a, pa });
    }

    /// emit all cfnodes
    for(auto& x : rfns)
    {
      auto v = x->to<Fn>();
      gccjit::block blk = fns[v] = gccfn.new_block(("l" + v->unique_name().get_string()).c_str());

      // load param from phi
      if(v->arg()->type()->kind() != NodeKind::Unit)
        blk.add_assignment(lvals[v->arg()].front(), rvals[v->arg()].back());

      // close lam
      if(v->bdy()->kind() == NodeKind::Case)
      {

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

        blk->end_with_return(cogen(vret.front(), blk));
      }
    }
    else
    {
      auto store_phi = [this,blk](Node::cRef param, Node::cRef arg)
        { blk->add_assignment(lvals[param].back(), rvals[arg].front()); };

      // If this is a basic block, we can simply jump
      if(v->caller()->type()->to<Fn>()->arg()->kind() == NodeKind::Fn)
      {
        assert(v->caller()->kind() == NodeKind::Fn && "Caller must be a fn."); // How the heck will this work if it is a param??
        store_phi(v->caller()->to<Fn>()->arg(), v->arg());

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

      auto lhs_typ = genty(bin->lhs());

      auto lhs = ctx.new_cast(cogen(bin->lhs(), blk), lhs_typ);
      auto rhs = ctx.new_cast(cogen(bin->rhs(), blk), lhs_typ);
      switch(bin->op)
      {
      case BinaryKind::Minus: return ctx.new_minus(lhs_typ, lhs, rhs);
      case BinaryKind::Plus:  return ctx.new_plus(lhs_typ, lhs, rhs);
      case BinaryKind::Mult:  return ctx.new_mult(lhs_typ, lhs, rhs);
      }
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
  //NodeMap<gccjit::block> fns;
  std::unordered_map<Node::cRef, gccjit::block, NodeHasher, NodeComparator> fns;
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


