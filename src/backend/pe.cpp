#include <backend/pe.hpp>
#include <backend/builder.hpp>

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
Fn::cRef drop(ir::builder& b, const ir::App* ref)
{
  // Move ref->arg() into ref->bdy() and yield ref->bdy()
}
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
        drop(b, ap);
        did_specialize = true;
      }
    }
    for(auto& s : succs(static_cast<const ir::Fn*>(hd_ref)))
      worklist.emplace(s);
  }
  return did_specialize;
}

