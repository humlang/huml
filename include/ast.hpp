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
  auto operator()(const T& arg) -> std::invoke_result_t<F, recursor<F>&&, const T&>
  { return f(*this, arg); }
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
struct assign;

// expressions
struct binary_exp;

using stmt_type = std::variant<std::monostate,
        rec_wrap_t<loop>,
        rec_wrap_t<block>,
        rec_wrap_t<assign>
>;

using exp_type = std::variant<std::monostate,
        rec_wrap_t<literal>,
        rec_wrap_t<identifier>,
        rec_wrap_t<binary_exp>
>;

using ast_type = std::variant<std::monostate,
      stmt_type,
      exp_type,
      rec_wrap_t<error>
>;

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
public:
    symbol symb() const { return tok.data; }
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

  const exp_type& num_times() const { return times; }
  const stmt_type& loop_body() const { return body; }

private:
  exp_type times;
  stmt_type body;
};

struct block : base<block>
{
public:
  block(tag, token tok, std::vector<std::variant<std::monostate, stmt_type, rec_wrap_t<error>>> stmts);
private:
  std::vector<std::variant<std::monostate, stmt_type, rec_wrap_t<error>>> stmts;
};

struct assign : base<assign>
{
public:
  assign(tag, rec_wrap_t<identifier> variable, token op, std::variant<std::monostate, exp_type, rec_wrap_t<error>> right);

  const exp_type& var() const { return variable; }
  const std::variant<std::monostate, exp_type, rec_wrap_t<error>>& exp() const { return right; }
private:
  exp_type variable;
  std::variant<std::monostate, exp_type, rec_wrap_t<error>> right;
};

struct binary_exp : base<binary_exp>
{
public:
  binary_exp(tag, std::variant<std::monostate, exp_type, rec_wrap_t<error>> left,
                  token op,
                  std::variant<std::monostate, exp_type, rec_wrap_t<error>> right);
  const std::variant<std::monostate, exp_type, rec_wrap_t<error>>& get_right_exp() const { return right;}
  const std::variant<std::monostate, exp_type, rec_wrap_t<error>>& get_left_exp() const { return left;}
private:
  std::variant<std::monostate, exp_type, rec_wrap_t<error>> left;
  std::variant<std::monostate, exp_type, rec_wrap_t<error>> right;
};

// Tag definitions
namespace ast_tags
{
    static constexpr inline tag<literal> literal = {};
    static constexpr inline tag<identifier> identifier = {};
    static constexpr inline tag<loop> loop = {};
    static constexpr inline tag<error> error = {};
    static constexpr inline tag<block> block = {};
    static constexpr inline tag<assign> assign = {};
    static constexpr inline tag<binary_exp> binary_exp = {};
}

