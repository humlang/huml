#pragma once

#include <token.hpp>

#include <variant>
#include <memory>

/// Needed to do std::visit
// https://en.cppreference.com/w/cpp/utility/variant/visit (16.10.19)
template<class... Ts> struct base_visitor : Ts... { using Ts::operator()...; };
template<class... Ts> base_visitor(Ts...) -> base_visitor<Ts...>;

template<typename F>
struct recursor
{
  recursor(F&& f) : f(f) {}

  template<typename T>
  void operator()(const T& arg)
  { f(*this, arg); }
private:
  F f;
};
template<typename F> recursor(F&&) -> recursor<F>;
// see sample for recursive visitor in ast_printer.hpp


template<typename T>
using rec_wrap_t = std::unique_ptr<T>;

// Tags are needed to make variant construction unique
namespace ast_tags
{
  template<typename T>
  struct tag {};
}

struct literal;
struct identifier;
struct loop;
struct error;
struct block;

using ast_type = std::variant<std::monostate,
      rec_wrap_t<literal>,
      rec_wrap_t<identifier>,
      rec_wrap_t<loop>,
      rec_wrap_t<error>,
      rec_wrap_t<block>
      >;

using stmt_type = std::variant<std::monostate,
      rec_wrap_t<loop>,
      rec_wrap_t<error>,
      rec_wrap_t<block>
      >;

inline static constexpr auto stmt_type_to_ast_type = base_visitor {
  [](stmt_type&& o)
  {
    if(std::holds_alternative<rec_wrap_t<loop>>(o))
      return ast_type(std::move(std::get<rec_wrap_t<loop>>(o)));
    else if(std::holds_alternative<rec_wrap_t<error>>(o))
      return ast_type(std::move(std::get<rec_wrap_t<error>>(o)));
    else if(std::holds_alternative<rec_wrap_t<block>>(o))
      return ast_type(std::move(std::get<rec_wrap_t<block>>(o)));
    std::exit(2);
  },
};

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

struct error : base<error>
{
public:
  error(tag, token tok);
};

struct loop : base<loop>
{
public:
  loop(tag, token tok, rec_wrap_t<literal> times, stmt_type body);

  const rec_wrap_t<literal>& num_times() const { return times; }
  const stmt_type& loop_body() const { return body; }

private:
  rec_wrap_t<literal> times;
  stmt_type body;
};

struct block : base<block>
{
public:
  block(tag, token tok, std::vector<stmt_type> stmts);
private:
  std::vector<stmt_type> stmts;
};


// Tag definitions
namespace ast_tags
{
  static constexpr inline tag<literal> literal = {};
  static constexpr inline tag<identifier> identifier = {};
  static constexpr inline tag<loop> loop = {};
  static constexpr inline tag<error> error = {};
  static constexpr inline tag<block> block = {};
}

