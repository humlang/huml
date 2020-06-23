#include <backend/pe.hpp>
#include <backend/builder.hpp>

#include <iostream>
#include <queue>

ir::App::cRef extract_app(ir::Node::cRef ref)
{
  auto hd_fn = static_cast<ir::Fn::cRef>(ref)->bdy();
  if(hd_fn->kind() != ir::NodeKind::App)
    return nullptr;
  return static_cast<ir::App::cRef>(hd_fn);
}

std::vector<ir::Fn::cRef> succs(ir::Fn::cRef ref)
{
  ir::NodeSet seen;
  std::queue<ir::Node::cRef> worklist;
  worklist.emplace(ref->bdy());
  seen.emplace(ref);

  std::vector<ir::Fn::cRef> ss;
  while(!worklist.empty())
  {
    auto hd = worklist.front(); worklist.pop();
    if(hd->kind() == ir::NodeKind::Fn)
    {
      ss.emplace_back(static_cast<ir::Fn::cRef>(hd));
      continue;
    }

    for(std::size_t i = 0, e = hd->argc(); i < e; ++i)
    {
      auto elem = hd->me()[i];
      if(seen.find(elem) == seen.end())
      {
        worklist.emplace(elem);
        seen.emplace(elem);
      }
    }
  }
  return ss;
}

namespace ir
{
/*
Fn::cRef drop(ir::builder& b, const ir::App* ref)
{
  tsl::robin_set<Node::cRef> seen;

  auto subst = [&b,&seen](auto subst, Node::cRef what, Node::cRef for_, Node::cRef in)
  {
    // Move ref->arg() into ref->bdy() and yield ref->bdy()
    if(what == for_)
      return in; // no-op

    ir::Node::cRef ret = ir::Node::no_ref;
    if(what == in)
      ret = for_;
    else
    {
      switch(in->kind())
      {
      case NodeKind::Kind:
      case NodeKind::Type:
      case NodeKind::Prop:
      case NodeKind::Unit:
      case NodeKind::Param:
      case NodeKind::Ctr:
        ret = in; break;

      case NodeKind::Fn: {
        Fn::cRef fn = static_cast<Fn::cRef>(in);
        if(fn->arg()->type() != nullptr)
          fn->arg()->set_type(subst(subst, what, for_, fn->arg()->type()));

        bool contains = seen.contains(in);
        seen.insert(in);

        // TODO: Fix pointer of nominals in case of recursion. perhaps we need a function that checks if fn is free in fn->bdy()
        auto b = contains ? fn->bdy() : subst(subst, what, for_, fn->bdy());

        ret = b.fn(fn->arg(), b);
      } break;

      case NodeKind::App: {
        App::cRef app = static_cast<App::cRef>(in);

        auto a = subst(subst, what, for_, app->caller());
        auto b = subst(subst, what, for_, app->arg());

        ret = b.app(a, b);
      } break;

      case NodeKind::Case: {
        Case::cRef cs = static_cast<Case::cRef>(in);

        auto of = subst(subst, what, for_, cs->of());
        auto arms = cs->match_arms();
        for(auto& match : arms)
        {
          if(match.first->type() != nullptr)
            match.first->set_type(subst(subst, what, for_, match.first->type()));
          match.second = subst(subst, what, for_, match.second);
        }
        ret = b.destruct(of, arms);
      } break;
      }
    }
    if(in->type() != nullptr)
      ret->type() = subst(subst, what, for_, in->type());

    return ret;
  };
  auto fnc = static_cast<Fn::cRef>(ref->caller());
  auto x = subst(subst, fnc->arg(), ref->arg(), fnc->bdy());

  return x;
}
*/
}

bool ir::partially_evaluate(ir::builder& b, const ir::Node* ref)
{
  bool did_specialize = false;

  NodeSet cache;

  std::queue<Node::cRef> worklist;
  worklist.emplace(ref);
  while(!worklist.empty())
  {
    auto hd_ref = worklist.front();
    if(hd_ref->kind() != ir::NodeKind::Fn)
      continue;

    auto head = extract_app(hd_ref); worklist.pop();
    if(head == nullptr)
      continue;
    auto bdy = head->caller();

    if(bdy->kind() == NodeKind::Fn)
    {
      // Functions can only have one arg
      Node::cRef arg = head->arg();

      // TODO: only fold if there is an annotation. For now, we drop any arg
      auto ap = static_cast<App::cRef>(b.app(bdy, arg));
      auto p = cache.emplace(ap);
      if(p.second)
      {
        std::cout << "dropping: \n";
        b.print_graph(std::cout, ap);
        std::cout << "\n\n";

        //drop(b, ap);
        did_specialize = true;
      }
    }
    for(auto& s : succs(static_cast<const ir::Fn*>(hd_ref)))
      worklist.emplace(s);
  }
  return did_specialize;
}

