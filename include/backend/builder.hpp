#pragma once

#include <backend/node.hpp>

namespace ir
{

struct builder
{
  builder();

  Node::cRef kind(); 
  Node::cRef type();
  Node::cRef prop();  
  Node::cRef unit();
  Node::cRef bot();
  Node::cRef i(bool no_sign, Node::cRef size);
  Node::cRef ptr(Node::cRef from);
  Node::cRef tup(std::vector<Node::cRef> elems);

  Node::cRef id(symbol symb, Node::cRef type);
  Node::cRef ignore();

  Node::cRef param(Node::cRef type = Node::no_ref);

  Node::cRef lit(std::uint_fast64_t value);
  Node::cRef binop(BinaryKind op, Node::cRef lhs, Node::cRef rhs);

  Fn::cRef fn(Node::cRef codomain, Node::cRef body, Node::cRef ret);

  Node::cRef app(Node::cRef caller, Node::cRef arg);

  Node::cRef destruct(Node::cRef of, std::vector<std::pair<Node::cRef, Node::cRef>> match_arms);


  std::ostream& print_graph(std::ostream& os, Node::cRef ref);
private:
  Node::Ref lookup_or_emplace(Node::Store store);
private:
  tsl::robin_set<Node::Store, NodeHasher, NodeComparator> nodes;
  tsl::robin_map<Fn::cRef, symbol, NodeHasher, NodeComparator> externals;

  std::uint_fast64_t gid { 0 };
};

}

