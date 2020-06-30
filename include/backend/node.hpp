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

  Ctr,
  Tup,

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

  template<typename T>
  const T* to() const { return static_cast<const T*>(this); }
  template<typename T>
  T* to() { return static_cast<T*>(this); }

  Node(NodeKind kind, std::vector<Node::cRef> children);

  NodeKind kind() const;
  std::size_t argc() const;
  std::uint_fast64_t gid() const;
  symbol unique_name() const;
  Node::cRef type() const;

  Node::cRef set_type(Node::cRef typ) { type_ = typ; return this; }

  virtual Node::cRef clone(builder& b) const
  { assert(false && "Cannot clone base node. Developer: Override this method!"); return nullptr; }

  const Node& me() const;
  const Node::cRef& operator[](std::size_t idx) const;
protected:
  NodeKind kind_;

  std::vector<Node::cRef> children_;
  Node::cRef type_ { no_ref };

  builder* mach_;
  std::uint_fast64_t gid_;
};

struct Param : Node
{
  using Ref = Param*;
  using cRef = Param*;

  Param(Node::cRef type)
    : Node(NodeKind::Param, {})
  { set_type(type); }

  Node::cRef clone(builder& b) const override;
};

struct Literal : Node
{
  using Ref = Literal*;
  using cRef = const Literal*;

  Literal(std::uint_fast64_t literal)
    : Node(NodeKind::Literal, {}), literal(literal)
  {  }

  Node::cRef clone(builder& b) const override;

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

  Binary(BinaryKind op, Node::cRef lhs, Node::cRef rhs)
    : Node(NodeKind::Binary, {lhs, rhs}), op(op)
  {  }

  Node::cRef clone(builder& b) const override;

  Node::cRef lhs() const { return me()[0]; }
  Node::cRef rhs() const { return me()[1]; }

  BinaryKind op;
};

// Only used for types, i.e. Nat, Vec, etc.
struct Constructor : Node
{
  using Ref = Constructor*;
  using cRef = const Constructor*;

  Constructor(symbol symb, Node::cRef type)
    : Node(NodeKind::Ctr, {}), name(symb)
  { set_type(type); }

  Node::cRef clone(builder& b) const override;

  symbol name;
};


struct Fn : Node
{
  using Ref = Fn*;
  using cRef = const Fn*;

  // domain is inferred from the body
  Fn(Node::cRef codomain, Node::cRef body)
    : Node(NodeKind::Fn, {codomain, body})
  {  }

  Node::cRef clone(builder& b) const override;

  Node::cRef arg() const { return me()[0]; }
  Node::cRef bdy() const { return me()[1]; }

  bool is_external() const
  { return external_name() != symbol(""); }

  symbol external_name() const
  { return external_name_; }

  void make_external(symbol name) const
  { external_name_ = name; }

private:
  mutable symbol external_name_ { "" };
};

struct App : Node
{
  using Ref = App*;
  using cRef = const App*;

  App(Node::cRef fn, Node::cRef param)
    : Node(NodeKind::App, {fn, param})
  {  }

  Node::cRef clone(builder& b) const override;

  Node::cRef caller() const { return me()[0]; }
  Node::cRef arg() const { return me()[1]; }
};

struct Case : Node
{
  using Ref = Case*;
  using cRef = const Case*;

  Case(Node::cRef of, std::vector<std::pair<Node::cRef, Node::cRef>> match_arms)
    : Node(NodeKind::Case, { of })
  {
    for(auto&& p : match_arms)
    {
      children_.emplace_back(std::move(p.first));
      children_.emplace_back(std::move(p.second));
    }
  }

  Node::cRef clone(builder& b) const override;

  Node::cRef of() const { return me()[0]; }
  std::vector<std::pair<Node::cRef, Node::cRef>> match_arms() const
  {
    std::vector<std::pair<Node::cRef, Node::cRef>> v;
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

  Node::cRef clone(builder& b) const override;
};

struct Type : Node
{
  using Ref = Type*;
  using cRef = const Type*;

  Type()
    : Node(NodeKind::Type, {})
  {  }

  Node::cRef clone(builder& b) const override;

  // TODO: add level to disallow Aczel Trees
};

struct Prop : Node
{
  using Ref = Prop*;
  using cRef = const Prop*;

  Prop()
    : Node(NodeKind::Prop, {})
  {  }

  Node::cRef clone(builder& b) const override;
};

struct Unit : Node
{
  using Ref = Unit*;
  using cRef = const Unit*;

  Unit()
    : Node(NodeKind::Unit, {})
  {  }

  Node::cRef clone(builder& b) const override;
};

struct Tup : Node
{
  using Ref = Tup*;
  using cRef = const Tup*;

  Tup(std::vector<Node::cRef> elems)
    : Node(NodeKind::Tup, std::move(elems))
  {  }

  Node::cRef clone(builder& b) const override;

  const std::vector<Node::cRef>& elements() const
  { return children_; }
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
using NodeSet = tsl::robin_set<Node::cRef, NodeHasher, NodeComparator>;

template<typename T>
using NodeMap = tsl::robin_map<Node::cRef, T, NodeHasher, NodeComparator>;

}

