#pragma once

#include <backend/node.hpp>

namespace ir
{

struct builder
{
  Node::Ref kind(); 
  Node::Ref type();
  Node::Ref prop();  
  Node::Ref id(symbol symb, Node::Ref type);

  Node::Ref param(Node::Ref type = Node::no_ref);

  Fn::Ref fn(Node::Ref domain, Node::Ref codomain);
  Fn::Ref fn();

  Node::Ref app(Node::Ref caller, Node::Ref callee);

  Node::Ref destruct(Node::Ref of, std::vector<std::pair<Node::Ref, Node::Ref>> match_arms);

  std::ostream& print(std::ostream& os, Node::Ref ref);
private:
  Node::Ref lookup_or_emplace(Node::Store store);

  tsl::robin_set<Node::Store, NodeHasher, NodeComparator> nodes;
};

}

