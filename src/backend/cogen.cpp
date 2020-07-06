#include <backend/cogen.hpp>
#include <backend/builder.hpp>
#include <backend/node.hpp>
#include <tmp.hpp>

#include <backend/sc.hpp>

#include <libgccjit++.h>

#include <iostream>
#include <queue>

namespace ir
{

std::vector<Node::cRef> find_reachable_fns(Node::cRef foo)
{
  std::queue<Node::cRef> work;
  work.emplace(foo);

  NodeSet seen;
  std::vector<Node::cRef> v;
  while(!work.empty())
  {
    auto f = work.front();
    work.pop();

    if(seen.contains(f))
      continue;
    seen.emplace(f);

    if(f->kind() == NodeKind::Fn)
      v.emplace_back(f);
    for(std::size_t i = 0; i < f->argc(); ++i)
      if(!seen.contains(f->me()[i]))
        work.emplace(f->me()[i]);
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
        case hash_string("i"): assert(ap->args().size() == 1 && "Wrong arity.");
                               return cogen_int(c, ap->args().front());
        // Pointer
        case hash_string("_Ptr"): {
            assert(ap->args().size() == 1 && "Wrong arity.");
            return genty(ap->args().front()).get_pointer();
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
    for(auto& v : node->to<Fn>()->args())
    {
      if(v->type()->kind() == NodeKind::Fn)
        ret = v->type();
    }
    // TODO: this assumes that the continuation is not hidden under a tuple. Should be ok if we enforce this somehow!
    assert(ret != nullptr && ret->kind() == NodeKind::Fn && "return continuation must exist and have a function type");

    /// Build function
    assert(ret->to<Fn>()->args().size() == 1 && "We only support one single return type atm");
    auto ret_type = genty(ret->to<Fn>()->args().front()->type());
    // TODO: check return type and potentially return a tuple
    
    std::vector<gccjit::param> params;
    for(auto& v : node->to<Fn>()->args())
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
    auto rfns = find_reachable_fns(node->to<Fn>());

    /// emit *all* (reachable) parameters and function blocks
    for(auto& x : rfns)
    {
      if((x == ret || fns.contains(x))
      || (x->to<Fn>()->bdy()->kind() == NodeKind::Ctr && x->to<Fn>()->bdy()->to<Constructor>()->name == symbol("_⊥")))
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
        cogen_handle_switch(v->bdy()->to<Case>(), &blk, ret->to<Fn>());
      }
      else if(v->bdy()->kind() == NodeKind::App)
      {
        cogen_handle_app(v->bdy()->to<App>(), &blk, ret->to<Fn>());
      }
      else
      {
        assert(false && "Cannot close block.");
      }
    }
  }
  void cogen_handle_switch(Case::cRef v, gccjit::block* blk, Fn::cRef ret)
  {
    // Case cannot return anything! It is not an rvalue!!
    auto oftyp = genty(v->of()->type());
    auto ofrval = cogen(v->of(), blk);

    // Generate blocks for all alternatives
    std::vector<gccjit::case_> alternatives;
    alternatives.reserve(v->match_arms().size());

    gccjit::block defblock;
    gccjit::block last_altblock;
    for(auto& x : v->match_arms())
    {
      if(x.first->kind() == NodeKind::Ctr && x.first->to<Constructor>()->name == symbol("_") && defblock.get_inner_block() == nullptr)
      {
        // generate default block
        gccjit::block def_block = blk->get_function().new_block(("b" + x.first->unique_name().get_string()).c_str());

        if(x.second->kind() == NodeKind::Case)
          cogen_handle_switch(x.second->to<Case>(), &def_block, ret);
        else if(x.second->kind() == NodeKind::App)
          cogen_handle_app(x.second->to<App>(), &def_block, ret);
        else
          assert(false && "Function must end with an app or case");

        defblock = def_block;
      }
      else
      {
        // generate a match arm
        gccjit::block alt_block = last_altblock = blk->get_function().new_block(("b" + x.first->unique_name().get_string()).c_str());

        if(x.second->kind() == NodeKind::Case)
          cogen_handle_switch(x.second->to<Case>(), &alt_block, ret);
        else if(x.second->kind() == NodeKind::App)
          cogen_handle_app(x.second->to<App>(), &alt_block, ret);
        else
          assert(false && "Function must end with an app or case");

        assert(x.first->kind() == NodeKind::Literal && "We only support literal patterns at the moment.");

        auto pat_val = ctx.new_rvalue(oftyp, static_cast<long>(x.first->to<Literal>()->literal));
        alternatives.emplace_back(ctx.new_case(pat_val, pat_val, alt_block));
      }
    }
    if(defblock.get_inner_block() == nullptr)
    {
      // Just jump to the first alternative
      assert(last_altblock.get_inner_block() != nullptr && "Cases must not be empty for codegen");

      defblock = blk->get_function().new_block(("b" + v->unique_name().get_string() + "_default").c_str());
      defblock.end_with_jump(last_altblock);
    }
    blk->end_with_switch(ofrval, defblock, alternatives);
  }
  void cogen_handle_app(App::cRef v, gccjit::block* blk, Fn::cRef ret)
  {
    if(v->caller()->type() == ret)
    {
      assert(v->args().size() != 0 && "Number of things to return must not be 0.");

      // just simply return
      if(v->args().size() == 1 && v->args().front()->type()->kind() == NodeKind::Unit)
        blk->end_with_return();
      else
      {
        assert(ret->args().size() == 1 && "We only support one elment returns currently.");

        auto ret_type = genty(ret->args().front()->type());
        blk->end_with_return(ctx.new_cast(cogen(v->args().front(), blk), ret_type));
      }
    }
    else
    {
      // If this is a basic block, we can simply jump
      if(v->caller()->kind() == NodeKind::Fn && !v->caller()->to<Fn>()->is_external())
      {
        auto x = v->caller()->to<Fn>();
        auto xargs = x->args();
        auto vargs = v->args();

        // Set all the parameters prior to function call
        assert(xargs.size() == vargs.size() && "Wrong arity");
        for(std::size_t i = 0; i < xargs.size(); ++i)
        {
          auto& a = xargs[i];
          auto& b = vargs[i];
          if(a->type()->kind() == NodeKind::Unit || a->type()->kind() == NodeKind::Fn)
            continue;
          cogen(b, blk);
          assert(lvals.contains(a) && rvals.contains(b) && "App args must exist in fn call.");

          blk->add_assignment(lvals[a].back(), ctx.new_cast(rvals[b].front(), genty(a->type())));
        }
        assert(fns.contains(x) && "Caller must be contained in the set.");
        blk->end_with_jump(fns[x]);
      }
      else
      {
        // TODO
        assert(false && "function calls of this kind are unimplemented");
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

      gccjit::rvalue ret;
      switch(bin->op)
      {
      case BinaryKind::Minus: ret = ctx.new_binary_op(GCC_JIT_BINARY_OP_MINUS, lhs_typ, lhs, rhs); break;
      case BinaryKind::Plus:  ret = ctx.new_binary_op(GCC_JIT_BINARY_OP_PLUS, lhs_typ, lhs, rhs);  break;
      case BinaryKind::Mult:  ret = ctx.new_binary_op(GCC_JIT_BINARY_OP_MULT, lhs_typ, lhs, rhs);
      }
      rvals.emplace(bin, std::vector<gccjit::rvalue>{ ret });

      return ret;
    }

    if(node->kind() == NodeKind::App || node->kind() == NodeKind::Case)
    {
      assert(false && "Unreachable, because kind app or case are not rvalues");
    }

    if(node->kind() == NodeKind::Literal)
    {
      auto ret = ctx.new_rvalue(ctx.get_type(GCC_JIT_TYPE_LONG), static_cast<long>(node->to<Literal>()->literal));
      rvals.emplace(node, std::vector<gccjit::rvalue>{ ret });

      return ret;
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

//  gen.ctx.set_bool_option(GCC_JIT_BOOL_OPTION_DUMP_INITIAL_GIMPLE, true);

  gen.ctx.compile_to_file(GCC_JIT_OUTPUT_KIND_EXECUTABLE, (name + ".out").c_str());
  return true;
}

}


