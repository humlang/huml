#pragma once

#include <backend/node.hpp>

namespace ir
{

struct builder
{
  Node::Ref kind()  { return lookup_or_emplace(Node::mk_node<Kind>()); }
  Node::Ref type()  { return lookup_or_emplace(Node::mk_node<Type>()); }
  Node::Ref prop()  { return lookup_or_emplace(Node::mk_node<Prop>()); }

  Node::Ref param() { return lookup_or_emplace(Node::mk_node<Param>()); }

  Node::Ref fn(Node::Ref domain, Node::Ref codomain) { return lookup_or_emplace(Node::mk_node<Fn>(domain, codomain)); }

  Node::Ref app(Node::Ref caller, Node::Ref callee)  { return lookup_or_emplace(Node::mk_node<App>(caller, callee)); }


  std::ostream& print(std::ostream& os, Node::Ref ref);

  Node::Ref lookup_or_emplace(Node::Store store)
  {
    if(auto it = nodes.find(store); it != nodes.end())
      return it->get(); // <- might be different pointer
    return nodes.emplace(std::move(store)).first->get();
  }

  tsl::robin_set<Node::Store, NodeHasher, NodeComparator> nodes;
};

}

