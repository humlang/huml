#pragma once

#include <backend/node.hpp>

namespace ir
{

struct builder
{
  builder();

  Node::Ref kind(); 
  Node::Ref type();
  Node::Ref prop();  
  Node::Ref unit();
  Node::Ref id(symbol symb, Node::Ref type);

  Node::Ref param(Node::Ref type = Node::no_ref);

  Fn::Ref fn(Node::Ref codomain, Node::Ref domain);
  Fn::Ref fn();

  Fn::Ref entry();
  Fn::Ref exit();

  Node::Ref app(Node::Ref caller, Node::Ref callee);

  Node::Ref destruct(Node::Ref of, std::vector<std::pair<Node::Ref, Node::Ref>> match_arms);


  // Prints anything that is reachable from main function
  void print(std::ostream& os);
private:
  std::ostream& print(std::ostream& os, Node::Ref ref);
private:
  Node::Ref lookup_or_emplace(Node::Store store);

  tsl::robin_set<Node::Store, NodeHasher, NodeComparator> nodes;

  Fn::Ref world_entry;
  Fn::Ref world_exit;
};

}

