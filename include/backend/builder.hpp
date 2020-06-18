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
  Node::Ref i(bool no_sign, Node::Ref size);

  Node::Ref id(symbol symb, Node::Ref type);
  Node::Ref ignore();

  Node::Ref param(Node::Ref type = Node::no_ref);

  Node::Ref lit(std::uint_fast64_t value);
  Node::Ref binop(BinaryKind op, Node::Ref lhs, Node::Ref rhs);

  Fn::Ref fn(Node::Ref codomain, Node::Ref domain);

  Fn::Ref entry();
  Fn::Ref exit();

  Node::Ref app(Node::Ref caller, Node::Ref arg);

  Node::Ref destruct(Node::Ref of, std::vector<std::pair<Node::Ref, Node::Ref>> match_arms);


  // Prints anything that is reachable from main function
  void print_graph(std::ostream& os);
  std::ostream& print_graph(std::ostream& os, Node::Ref ref);

private:
  Node::Ref lookup_or_emplace(Node::Store store);
private:
  tsl::robin_set<Node::Store, NodeHasher, NodeComparator> nodes;

  Fn::Ref world_entry;
  Fn::Ref world_exit;

  std::uint_fast64_t gid { 0 };
};

}

