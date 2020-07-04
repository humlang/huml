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
  static bool run(ir::builder& b, ir::Node::cRef ref)
  {
    SupervisingCompiler sc(b);
    return sc.run(ref);
  }
private:
  bool run(ir::Node::cRef ref)
  {
    if(ref->kind() == NodeKind::Fn)
      worklist.emplace(ref->to<Fn>());
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
      
      // Specialize case and app calls in case there is something to specialize
      Node::cRef new_body = nullptr;
      if(fn->bdy()->kind() == NodeKind::App)
        new_body = handle_app(fn->bdy()->to<App>());
      else
        new_body = handle_case(fn->bdy()->to<Case>());

      if(fn->bdy() != new_body)
      {
        // change fns body
        auto new_fn = b.subst(fn->bdy(), new_body, fn);
        
        assert(new_fn->kind() == NodeKind::Fn && "Must still be a function after subst.");

        worklist.emplace(new_fn->to<Fn>());
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
      auto new_caller = app->caller()->to<Fn>()->bdy()->clone(b);
      auto caller_args = app->caller()->to<Fn>()->args();
      auto app_args = app->args();
      assert(caller_args.size() == app_args.size() && "Same arity required.");

      // For each position that we can specialize:
      //   subst the arg into the body of a clone of the caller
      for(std::size_t idx : can_be_specialized)
      {
        assert(caller_args[idx]->kind() == NodeKind::ConstexprAnnot && "To be specialized arg should be annotated.");

        Node::cRef x = b.subst(caller_args[idx], app_args[idx], new_caller);

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
      }
      // create a new version of new_caller with the fresh args
      auto new_fn = b.fn(new_args, new_caller);

      // Finally, construct a fresh `app` adhering the new interface
      auto new_app = b.app(new_fn, new_app_args);

      fill_worklist_with_children(new_app);
      return new_app;
    }
    else
      fill_worklist_with_children(app);
    return app;
  }

  ir::Node::cRef handle_case(ir::Case::cRef case_)
  {
    // Currently, we do nothing. However, as soon as the supercompiler gets going, this will be a core part of it.
    // We analyze if we can split using case_->of() etc.

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
  std::queue<Fn::cRef> worklist;
};
}

bool ir::supercompile(ir::builder& b, const ir::Node* ref)
{
  return SupervisingCompiler::run(b, ref);
}

