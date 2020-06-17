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

struct Node
{
  using Ref  = Node*;
  using cRef = const Node*;
  using Store = std::unique_ptr<Node>;

  constexpr static Ref no_ref = nullptr;

  template<typename Type, typename... Args>
  static Store mk_node(Args&& ...args)
  { return std::make_unique<Type>(std::forward<Args>(args)...); }

  Node(NodeKind kind, std::size_t argc)
    : kind(kind), nominal_(true), argc(argc)
  { children.resize(argc, nullptr); }

  Node(NodeKind kind, std::vector<Node::Ref> children)
    : kind(kind), nominal_(false), argc(children.size()), children(children)
  {  }

  bool nominal() const
  { return nominal_; }

  void set_type(Node::Ref typ) { type = typ; }

  Node::Ref& operator[](std::size_t idx) { return children[idx]; }
  const Node::cRef& operator[](std::size_t idx) const { return children[idx]; }

  Node& me() { return *this; }
  const Node& me() const { return *this; }

  NodeKind kind;

  bool nominal_ : 1;
  std::size_t argc;
  std::vector<Node::Ref> children;
  Node::Ref type { no_ref };
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


//thread_local std::size_t fn_count = 0;

// Implicitly returns BOT
struct Fn : Node
{
  using Ref = Fn*;
  using cRef = const Fn*;

  Fn(Node::Ref codomain, Node::Ref domain)
    : Node(NodeKind::Fn, {codomain, domain})
  {  }

  Fn()
    : Node(NodeKind::Fn, 2)
  {  }

  Node::Ref arg()  { return me()[0]; }
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
  { children.emplace_back(fn); children.emplace_back(param); }

  Node::Ref caller() const { return children.front(); }
  Node::Ref arg() const { return children.back(); }
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
      children.emplace_back(std::move(p.first));
      children.emplace_back(std::move(p.second));
    }
    argc = children.size();
  }

  Node::Ref of() { return me()[0]; }
  std::vector<std::pair<Node::Ref, Node::Ref>> match_arms()
  {
    std::vector<std::pair<Node::Ref, Node::Ref>> v;
    v.reserve(argc - 1);
    for(std::size_t i = 1; i < argc - 1; i += 2)
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
  std::size_t operator()(const Node::cRef ref) const
  {
    switch(ref->kind)
    {
    case NodeKind::Kind: return 1;
    case NodeKind::Prop: return 2;
    case NodeKind::Type: return 3;
    case NodeKind::Unit: return 4;
    case NodeKind::Param: return (reinterpret_cast<std::size_t>(&*ref) << 7) + 0x9e3779b9;
    case NodeKind::Ctr:
      return static_cast<Constructor::cRef>(ref)->name.get_hash();

    case NodeKind::Int: return (8 << 8) + (*this)(static_cast<Int::cRef>(ref)->me()[0]);
    case NodeKind::Literal: return 0x9e3779b9 ^ static_cast<Literal::cRef>(ref)->literal;

    case NodeKind::Binary: {
        auto r = static_cast<Binary::cRef>(ref);

        auto lh = (*this)(r->me()[0]);
        auto rh = (*this)(r->me()[1]);

        return 11 + lh + 0x9e3779b9 + (rh << 6) + (rh >> 2);
      } break;


    case NodeKind::App: {
        const App::cRef app = static_cast<App::cRef>(ref);

        std::size_t seed = 5 + (*this)(app->me()[0]);
        return 3301 + (*this)(app->me()[1]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      } break;

    case NodeKind::Fn: return reinterpret_cast<std::size_t>(&*ref) << 13;
    
    case NodeKind::Case: {
        std::size_t hash = 4 + 0x9e3779b9 + ((*this)(ref->me()[0]) << 6);
        for(std::size_t i = 1; i < ref->argc; ++i)
          hash = (*this)(ref->me()[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        return hash;
      } break;
    }
    assert(false && "Unhandled case.");
    return 0;
  }
};
struct NodeComparator
{
  bool operator()(const Node::Store& lhs, const Node::Store& rhs) const
  { return (*this)(lhs.get(), rhs.get()); }
  bool operator()(const Node::cRef lhs, const Node::cRef rhs) const
  {
    if(lhs->kind != rhs->kind)
      return false;

    switch(lhs->kind)
    {
    case NodeKind::Kind:
    case NodeKind::Type:
    case NodeKind::Prop:
    case NodeKind::Unit:
      return true;

    case NodeKind::Int:
      return (*this)(static_cast<Int::cRef>(lhs)->me()[0], static_cast<Int::cRef>(rhs)->me()[0]);
    case NodeKind::Literal:
      return static_cast<Literal::cRef>(lhs)->literal == static_cast<Literal::cRef>(rhs)->literal;

    case NodeKind::Binary: {
        auto l = static_cast<Binary::cRef>(lhs);
        auto r = static_cast<Binary::cRef>(rhs);

        return (*this)(l->me()[0], r->me()[0]) && (*this)(l->me()[1], r->me()[1]);
      } break;

      // Same pointer is same binding
    case NodeKind::Param:
      return lhs == rhs; 

    case NodeKind::Ctr:
      return static_cast<Constructor::cRef>(lhs)->name.get_hash() == static_cast<Constructor::cRef>(rhs)->name.get_hash();
    
    case NodeKind::Case: {
        if(lhs->argc != rhs->argc)
          return false;
        for(std::size_t i = 0; i < lhs->argc; ++i)
          if(!((*this)(lhs->me()[i], rhs->me()[i])))
            return false;
        return true;
      } break;

    case NodeKind::Fn:
      if(lhs == rhs)
        return true;
      return !lhs->nominal() && !rhs->nominal() && lhs->me()[0] == rhs->me()[0] && lhs->me()[1] == rhs->me()[1];
    case NodeKind::App:
        return (*this)(lhs->me()[0], rhs->me()[0]) && (*this)(lhs->me()[1], rhs->me()[1]);
    }
    assert(false && "Unhandled case.");
    return false;
  }
};
using NodeSet = tsl::robin_set<Node::Ref, NodeHasher, NodeComparator>;

template<typename T>
using NodeMap = tsl::robin_map<Node::Ref, T, NodeHasher, NodeComparator>;

}

