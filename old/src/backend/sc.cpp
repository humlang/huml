#include <backend/builder.hpp>
#include <backend/sc.hpp>

#include <iostream>
#include <queue>

namespace ir
{
struct SupervisingCompiler
{
private:
  SupervisingCompiler(ir::builder& b)
    : b(b)
  {  }
public:
  static bool run(builder& b, Node::cRef ref)
  {
    SupervisingCompiler sc(b);
    return sc.run(ref);
  }
private:
  bool run(Node::cRef ref)
  {
    if(ref->kind() == NodeKind::Fn)
      worklist.emplace(ref->to<Fn>());
    if(ref->kind() == NodeKind::Root)
      for(std::size_t i = 0, e = ref->to<Root>()->argc(); i < e; ++i)
        if(ref->me()[i]->kind() == NodeKind::Fn)
          worklist.emplace(ref->me()[i]->to<Fn>());
    while(!worklist.empty())
    {
      auto hd_ref = worklist.front();
      worklist.pop();
      if(hd_ref->kind() != NodeKind::Fn || cache.contains(hd_ref))
        continue; // skip non-functions and things we've seen before
      auto fn = hd_ref->to<Fn>();
      cache.insert(fn);
      assert((fn->bdy()->kind() == NodeKind::App || fn->bdy()->kind() == NodeKind::Case)
           && "Fn needs to be closed with app or case.");

      // NOTE: we must not ignore externals, they contain app/cases that we certainly want to specialize!
      
      //if(cache.contains(fn))
      //  continue;
      // Specialize case and app calls in case there is something to specialize
      Node::cRef new_body = nullptr;
      if(fn->bdy()->kind() == NodeKind::App)
        new_body = handle_app(fn->bdy()->to<App>());
      else
        new_body = handle_case(fn->bdy()->to<Case>());

      if(fn->bdy() != new_body)
      {
        // change fn's body
        auto new_fn = b.subst(fn->bdy(), new_body, fn);
        
        assert(new_fn->kind() == NodeKind::Fn && "Must still be a function after subst.");

        worklist.emplace(new_fn->to<Fn>());

        if(auto it = cache.find(new_fn); it != cache.end())
          cache.erase(it);
      }
    }
    return did_specialize;
  }

  // Looks at this call site, perhaps generates a new function if an argument shall be partially specialized.
  //  e.g.:
  //    ```
  //    incAll(xs : [Int], ret : [Int] -> ⊥) -> ⊥:
  //      map (\x. x + 1) xs ret
  //    ```
  //    becomes a specialized call to map with "inlined" `\x. x + 1` and `ret`.
  //
  // Finally, we need to collect any kind of function and put it in our worklist
  ir::Node::cRef handle_app(ir::App::cRef app)
  {
    // Can't do anything about apps calling constuctors or params
    if(app->caller()->kind() == NodeKind::App && app->caller()->to<App>()->caller() == b.rec())
    {
      assert(app->caller()->to<App>()->args().size() == 1 && "Z combinator expects exactly one arg");

      auto args = app->args();
      args.insert(args.begin(), app->caller());

      // (Z f) a b c   -> f (Z f) a b c
      auto ap = b.app(app->caller()->to<App>()->args().front(), args);
      assert(ap->kind() == NodeKind::App && "Must be an app again");

      // retry
      return ap;
    }
    else if(app->caller()->kind() != NodeKind::Fn)
    {
      fill_worklist_with_children(app);
      return app;
    }
    // Look at caller arguments and see if we can specialize a position.
    std::vector<std::size_t> can_be_specialized;
    can_be_specialized.reserve(app->argc() - 1);
    { std::size_t idx = 0;
    for(auto& a : app->caller()->to<Fn>()->args())
    {
      if(a->kind() == NodeKind::ConstexprAnnot)
        can_be_specialized.emplace_back(idx);
      ++idx;
    }
    }
    if(!can_be_specialized.empty())
    {
      auto new_caller = app->caller()->to<Fn>()->clone(b, specialized_params);
      auto caller_args = new_caller->to<Fn>()->args();
      auto app_args = app->args();
      assert(caller_args.size() == app_args.size() && "Same arity required.");

      // For each position that we can specialize:
      //   subst the arg into the body of a clone of the caller
      for(std::size_t idx : can_be_specialized)
      {
        assert(caller_args[idx]->kind() == NodeKind::ConstexprAnnot && "To be specialized arg should be annotated.");

        Node::cRef x = b.subst(caller_args[idx], app_args[idx], new_caller);

        assert(b.is_free(caller_args[idx], new_caller) && "caller arg must not exist anymore");
        assert(new_caller == x && "subst should only substitute inside the body of new_caller");
      }
      // Now, we need to change the interface of new_caller, i.e. remove the specialized args
      std::vector<Node::cRef> new_args, new_app_args;
      for(std::size_t i = 0, e = caller_args.size(); i < e; ++i)
      {
        if(std::binary_search(can_be_specialized.begin(), can_be_specialized.end(), i) == false)
        {
          new_args.emplace_back(caller_args[i]);
          new_app_args.emplace_back(app_args[i]);
        }
        else
        {
          assert(b.is_free(caller_args[i], new_caller) && "Caller arg must appear free");
        }
      }
      if(new_args.empty() && new_app_args.empty())
      {
        new_args.push_back(b.unit());
        new_app_args.push_back(b.unit());
      }
      // create a new version of new_caller with the fresh args
      auto new_fn = b.fn(new_args, new_caller->to<Fn>()->bdy());

      // Finally, construct a fresh `app` adhering the new interface
      auto new_app = b.app(new_fn, new_app_args);
      assert(new_app->kind() == NodeKind::App && "Must be an app.");

      app = new_app->to<App>();
    }
    fill_worklist_with_children(app);
    return app;
  }

  ir::Node::cRef handle_case(ir::Case::cRef case_)
  {
    // Currently, we do nothing. However, as soon as the supercompiler gets going, this will be a core part of it.
    // We analyze if we can split using case_->of() etc.


    auto arms = case_->match_arms();
    for(std::size_t i = 0; i < arms.size(); ++i)
    {
      if(arms[i].second->kind() == NodeKind::Case)
      {
        auto new_arm = handle_case(arms[i].second->to<Case>());

        if(arms[i].second != new_arm)
          b.subst(arms[i].second, new_arm, case_);
      }
      else if(arms[i].second->kind() == NodeKind::App)
      {
retry:
        auto new_arm = handle_app(arms[i].second->to<App>());

        if(arms[i].second != new_arm)
        {
          b.subst(arms[i].second, new_arm, case_);

          arms = case_->match_arms();
          if(new_arm->kind() == NodeKind::App)
            goto retry;
        }
      }
    }

    fill_worklist_with_children(case_);
    return case_;
  }

  void fill_worklist_with_children(ir::Node::cRef node)
  {
    for(std::size_t i = 0, e = node->argc(); i < e; ++i)
    {
      auto kind = node->me()[i]->kind();
      if(kind == NodeKind::Fn)
        worklist.push(node->me()[i]->to<Fn>());
      else if(kind == NodeKind::App || kind == NodeKind::Case)
        fill_worklist_with_children(node->me()[i]); // needed for match arms
    }
  }

private:
  bool did_specialize { false };
  ir::NodeSet cache;
  ir::builder& b;

  NodeMap<Node::cRef> specialized_params;

  std::queue<Fn::cRef> worklist;
};
}

bool ir::supercompile(ir::builder& b, const ir::Node* ref)
{
  return SupervisingCompiler::run(b, ref);
}

