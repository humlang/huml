#include <backend/builder.hpp>
#include <backend/sc.hpp>

#include <iostream>
#include <queue>

bool ir::supercompile(ir::builder& b, const ir::Node* ref)
{
  bool did_specialize = false;

  NodeSet cache;

  std::queue<Node::cRef> worklist;
  worklist.emplace(ref);
  while(!worklist.empty())
  {
    auto hd_ref = worklist.front();
    // hd_ref should be a case or an app

    // TODO: implement
  }
  return did_specialize;
}

