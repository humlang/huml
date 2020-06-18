#pragma once

#include <tsl/robin_map.h>
#include <tsl/robin_set.h>
#include <symbol.hpp>
#include <atomic>
#include <memory>
#include <vector>

namespace ir
{

enum class NodeKind
{
  Fn,
  Param,
  App,
  Case,
  Binary,

  Int,
  Ctr,

  Literal,

  Type,
  Prop,
  Kind,
  Unit,
};

struct builder;

struct Node
{
  friend struct builder;

  using Ref  = Node*;
  using cRef = const Node*;
  using Store = std::unique_ptr<Node>;

  constexpr static Ref no_ref = nullptr;

  template<typename Type, typename... Args>
  static Store mk_node(Args&& ...args)
  { return std::make_unique<Type>(std::forward<Args>(args)...); }

  Node(NodeKind kind, std::size_t argc);
  Node(NodeKind kind, std::vector<Node::Ref> children);

  NodeKind kind() const;
  bool nominal() const;
  std::size_t argc() const;
  std::uint_fast64_t gid() const;
  Node::Ref type() const;

  void set_type(Node::Ref typ) { type_ = typ; }

  Node& me();
  Node::Ref& operator[](std::size_t idx);

  const Node& me() const;
  const Node::cRef& operator[](std::size_t idx) const;
protected:
  NodeKind kind_;

  bool nominal_ : 1;
  std::size_t argc_;
  std::vector<Node::Ref> children_;
  Node::Ref type_ { no_ref };

  builder* mach_;
  std::uint_fast64_t gid_;
};

struct Param : Node
{
  using Ref = Param*;
  using cRef = Param*;

  Param(Node::Ref type)
    : Node(NodeKind::Param, {})
  { set_type(type); }
};

struct Int : Node
{
  using Ref = Int*;
  using cRef = const Int*;

  Int(bool no_sign, Node::Ref size)
    : Node(NodeKind::Int, {size}), no_sign(no_sign)
  {  }

  Node::cRef size() const { return me()[0]; }
  Node::Ref size() { return me()[0]; }
  bool is_unsigned() const { return no_sign; }

  bool no_sign;
};

struct Literal : Node
{
  using Ref = Literal*;
  using cRef = const Literal*;

  Literal(std::uint_fast64_t literal)
    : Node(NodeKind::Literal, {}), literal(literal)
  {  }

  std::uint_fast64_t literal;
};

enum class BinaryKind : std::int_fast8_t
{
  Plus  = '+',
  Minus = '-',
  Mult  = '*',
};
struct Binary : Node
{
  using Ref = Binary*;
  using cRef = const Binary*;

  Binary(BinaryKind op, Node::Ref lhs, Node::Ref rhs)
    : Node(NodeKind::Binary, {lhs, rhs}), op(op)
  {  }

  Node::cRef lhs() const { return me()[0]; }
  Node::cRef rhs() const { return me()[1]; }

  Node::Ref lhs() { return me()[0]; }
  Node::Ref rhs() { return me()[1]; }

  BinaryKind op;
};

// Only used for types, i.e. Nat, Vec, etc.
struct Constructor : Node
{
  using Ref = Constructor*;
  using cRef = const Constructor*;

  Constructor(symbol name, Node::Ref type)
    : Node(NodeKind::Ctr, {}), name(name)
  { set_type(type); }

  symbol name;
};


struct Fn : Node
{
  using Ref = Fn*;
  using cRef = const Fn*;

  Fn(Node::Ref codomain, Node::Ref domain)
    : Node(NodeKind::Fn, {codomain, domain})
  { this->nominal_ = true; }

  Node::cRef arg() const { return me()[0]; }
  Node::cRef bdy() const { return me()[1]; }

  Node::Ref arg() { return me()[0]; }
  Node::Ref bdy() { return me()[1]; }

  Node::Ref set_arg(Node::Ref arg) { return me()[0] = arg; }
  Node::Ref set_bdy(Node::Ref bdy) { return me()[1] = bdy; }
};

struct App : Node
{
  using Ref = App*;
  using cRef = const App*;

  App(Node::Ref fn, Node::Ref param)
    : Node(NodeKind::App, {fn, param})
  {  }

  Node::cRef caller() const { return me()[0]; }
  Node::cRef arg() const { return me()[1]; }

  Node::Ref caller() { return me()[0]; }
  Node::Ref arg() { return me()[1]; }
};

struct Case : Node
{
  using Ref = Case*;
  using cRef = const Case*;

  Case(Node::Ref of, std::vector<std::pair<Node::Ref, Node::Ref>> match_arms)
    : Node(NodeKind::Case, { of })
  {
    for(auto&& p : match_arms)
    {
      children_.emplace_back(std::move(p.first));
      children_.emplace_back(std::move(p.second));
    }
    argc_ = children_.size();
  }

  Node::cRef of() const { return me()[0]; }
  std::vector<std::pair<Node::cRef, Node::cRef>> match_arms() const
  {
    std::vector<std::pair<Node::cRef, Node::cRef>> v;
    v.reserve(argc() - 1);
    for(std::size_t i = 1, e = argc(); i < e - 1; i += 2)
      v.emplace_back(me()[i], me()[i + 1]);
    return v;
  }

  Node::Ref of() { return me()[0]; }
  std::vector<std::pair<Node::Ref, Node::Ref>> match_arms()
  {
    std::vector<std::pair<Node::Ref, Node::Ref>> v;
    v.reserve(argc() - 1);
    for(std::size_t i = 1, e = argc(); i < e - 1; i += 2)
      v.emplace_back(me()[i], me()[i + 1]);
    return v;
  }
};

struct Kind : Node
{
  using Ref = Kind*;
  using cRef = const Kind*;

  Kind()
    : Node(NodeKind::Kind, {})
  {  }

  // TODO: add level to disallow Aczel Trees
};

struct Type : Node
{
  using Ref = Type*;
  using cRef = const Type*;

  Type()
    : Node(NodeKind::Type, {})
  {  }
};

struct Prop : Node
{
  using Ref = Prop*;
  using cRef = const Prop*;

  Prop()
    : Node(NodeKind::Prop, {})
  {  }
};

struct Unit : Node
{
  using Ref = Unit*;
  using cRef = const Unit*;

  Unit()
    : Node(NodeKind::Unit, {})
  {  }
};

struct NodeHasher
{
  std::size_t operator()(const Node::Store& str) const
  { return (*this)(str.get()); }
  std::size_t operator()(const Node::cRef ref) const;
};
struct NodeComparator
{
  bool operator()(const Node::Store& lhs, const Node::Store& rhs) const
  { return (*this)(lhs.get(), rhs.get()); }
  bool operator()(const Node::cRef lhs, const Node::cRef rhs) const;
};
using NodeSet = tsl::robin_set<Node::Ref, NodeHasher, NodeComparator>;

template<typename T>
using NodeMap = tsl::robin_map<Node::cRef, T, NodeHasher, NodeComparator>;

}

