#pragma once

#include <token.hpp>
#include <ast.hpp>

#include <variant>
#include <memory>

// Tags are needed to make variant construction unique
namespace ast_tags
{
  template<typename T>
  struct tag {};
}

// This aggregate type is used to add basic information to AST nodes
template<typename T>
struct base
{
public:
  using tag = ast_tags::tag<T>;
public:
  static std::uint64_t counter;
  std::uint64_t _id { counter++ };
  token tok;
protected:
  base(tag, token tok)
    : tok(tok)
  { }

  std::uint64_t id() const { return _id; }
  symbol symb() const { return tok.data; }
public:
  source_range loc() { return tok.loc; }
private:
        T& self()       { return static_cast<T&>(*this); }
  const T& self() const { return static_cast<const T&>(*this); }
};
template<typename T>
std::uint64_t base<T>::counter = 0;

// Nodes
struct literal : base<literal>
{
public:
  literal(tag, token tok);
};

struct identifier : base<identifier>
{
public:
  identifier(tag, token tok);
};


// Final AST-Type
using ast_type = std::variant<std::monostate,
      literal,
      identifier>;

// Tag definitions
namespace ast_tags
{
  static constexpr inline tag<literal> literal = {};
  static constexpr inline tag<identifier> identifier = {};
}

