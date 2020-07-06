#pragma once

#include <backend/node.hpp>
#include <unordered_set>

namespace ir
{

struct builder
{
  friend struct Constructor;
  friend struct Fn;

  builder();

  Node::cRef kind(); 
  Node::cRef type();
  Node::cRef prop();  
  Node::cRef unit();
  Node::cRef bot();
  Node::cRef i(bool no_sign, Node::cRef size);
  Node::cRef ptr(Node::cRef from);
  Node::cRef tup(std::vector<Node::cRef> elems);

  // constructs the Z combinator used for recursion
  Node::cRef rec();
  Node::cRef id(symbol symb, Node::cRef type);
  Node::cRef ignore();

  Node::cRef param(Node::cRef type = Node::no_ref);
  Node::cRef cexpr(Node::cRef expr); // <-- param `expr` is considered constant

  Node::cRef lit(std::uint_fast64_t value);
  Node::cRef binop(BinaryKind op, Node::cRef lhs, Node::cRef rhs);

  Node::cRef entry_ret();
  Fn::cRef fn(std::vector<Node::cRef> args, Node::cRef body);

  Node::cRef app(Node::cRef caller, std::vector<Node::cRef> arg);

  Node::cRef destruct(Node::cRef of, std::vector<std::pair<Node::cRef, Node::cRef>> match_arms);


  bool is_free(Node::cRef what, Node::cRef in);

  /// Replaces `what` with `with` in *all* occurences. -> what will be erased
  Node::cRef subst(Node::cRef what, Node::cRef with, Node::cRef in);

  std::ostream& print(std::ostream& os, Node::cRef ref);
  std::ostream& print_graph(std::ostream& os, Node::cRef ref);
private:
  Node::Ref lookup_or_emplace(Node::Store store);
private:
  std::vector<Node::Store> data;
  NodeMap<std::size_t> nodes;

  std::uint_fast64_t gid { 0 };
};

}

