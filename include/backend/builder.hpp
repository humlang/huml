#pragma once

#include <backend/node.hpp>

namespace ir
{

struct builder
{
  Node::Ref kind()  { return lookup_or_emplace(Node::mk_node<Kind>()); }
  Node::Ref type()  { return lookup_or_emplace(Node::mk_node<Type>()); }
  Node::Ref prop()  { return lookup_or_emplace(Node::mk_node<Prop>()); }
  Node::Ref id(symbol symb, Node::Ref type) { assert(type != Node::no_ref && "Type must exist."); return lookup_or_emplace(Node::mk_node<Identifier>(symb, type)); }

  Node::Ref param(Node::Ref type = Node::no_ref) { return lookup_or_emplace(Node::mk_node<Param>(type)); }

  Fn::Ref fn(Node::Ref domain, Node::Ref codomain) { return static_cast<Fn::Ref>(lookup_or_emplace(Node::mk_node<Fn>(domain, codomain))); }
  Fn::Ref fn() { return static_cast<Fn::Ref>(lookup_or_emplace(Node::mk_node<Fn>())); }

  Node::Ref app(Node::Ref caller, Node::Ref callee)  { return lookup_or_emplace(Node::mk_node<App>(caller, callee)); }

  Node::Ref destruct(Node::Ref of, std::vector<std::pair<Node::Ref, Node::Ref>> match_arms)
  { return lookup_or_emplace(Node::mk_node<Case>(of, match_arms)); }

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

