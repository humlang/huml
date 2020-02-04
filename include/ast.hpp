#pragma once

#include <atomic>
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

template<typename S, typename F>
struct stateful_recursor
{
  stateful_recursor(S&& s, F&& f) : state(s), f(f) {}

  template<typename T>
  auto operator()(const T& arg) -> std::invoke_result_t<F, stateful_recursor<S, F>&&, const T&>
  { return f(*this, arg); }

  S state;
private:
  F f;
};
template<typename S, typename F>
F& stateful_recursor_friend_hack(stateful_recursor<S, F>& r)
{ return r.f; }

template<typename S, typename F> stateful_recursor(S&&, F&&) -> stateful_recursor<S, F>;
// see sample for recursive visitor in ast_printer.hpp


template<typename T>
using rec_wrap_t = std::unique_ptr<T>;

// Tags are needed to make variant construction unique
namespace ast_tags
{
  template<typename T>
  struct tag
  {
    // can be convenient.
    template<typename... Args>
    static rec_wrap_t<T> make_node(Args&&... args)
    { return std::make_unique<T>(tag<T>(), std::forward<Args>(args)...); }
  };
}

struct literal_;    using literal = rec_wrap_t<literal_>;
struct identifier_; using identifier = rec_wrap_t<identifier_>;
struct loop_;       using loop = rec_wrap_t<loop_>;
struct error_;      using error = rec_wrap_t<error_>;
struct block_;      using block = rec_wrap_t<block_>;
struct assign_;     using assign = rec_wrap_t<assign_>;

// expressions
struct binary_exp_; using binary_exp = rec_wrap_t<binary_exp_>;

using stmt_type = std::variant<std::monostate,
        loop,
        block,
        assign
>;

using exp_type = std::variant<std::monostate,
        literal,
        identifier,
        binary_exp
>;

using ast_type = std::variant<std::monostate,
      stmt_type,
      exp_type,
      error
>;
// The error node is special, as it can also be an expression or a statement.
// To implement this in a somewhat clean way (visitors will do fine with this)
// we introduce "optional" types, where the content is either a exp/stmt or an error
//  -> This forces us to really handle errors somewhat correctly in the parser
using maybe_expr = std::variant<std::monostate, exp_type, rec_wrap_t<error_>>;
using maybe_stmt = std::variant<std::monostate, stmt_type, rec_wrap_t<error_>>;

// POLICY: Every getter must return one of the previously declared variants.
//          Otherwise, have fun parsing the error messages coming from our visitors

// This aggregate type is used to add basic information to AST nodes

// we need this, otherwise the counter will only be unique per ast-node-type
//   i.e. 1 + 2 yields ids "0, 0, 1", where the last two are for the literals
struct unique_base
{
public:
  std::uint64_t id() const { return _id; }
private:
  static std::atomic_uint_fast64_t counter;
  std::uint64_t _id { counter++ };
};

template<typename T>
struct base : unique_base
{
public:
  using tag = ast_tags::tag<T>;
public:
  token tok;
protected:
  base(tag, token tok)
    : unique_base(), tok(tok)
  { }

public:
  symbol symb() const { return tok.data; }
  source_range loc() { return tok.loc; }
private:
        T& self()       { return static_cast<T&>(*this); }
  const T& self() const { return static_cast<const T&>(*this); }
};

// Nodes
struct literal_ : base<literal_>
{
public:
  literal_(tag, token tok);
};

struct identifier_ : base<identifier_>
{
public:
  identifier_(tag, token tok);
};

struct error_ : base<error_>
{
public:
  error_(tag, token tok);
};

struct loop_ : base<loop_>
{
public:
  loop_(tag, token tok, maybe_expr times, maybe_stmt body);

  const maybe_expr& num_times() const { return times; }
  const maybe_stmt& loop_body() const { return body; }

private:
  maybe_expr times;
  maybe_stmt body;
};

struct block_ : base<block_>
{
public:
  block_(tag, token tok, std::vector<maybe_stmt> stmts);

  const std::vector<maybe_stmt>& statements() const { return stmts; }
private:
  std::vector<maybe_stmt> stmts;
};

struct assign_ : base<assign_>
{
public:
  assign_(tag, identifier variable, token op, maybe_expr right);

  const exp_type& var() const { return variable; }
  const maybe_expr& exp() const { return right; }
private:
  exp_type variable;
  maybe_expr right;
};

struct binary_exp_ : base<binary_exp_>
{
public:
  binary_exp_(tag, maybe_expr left, token op, maybe_expr right);

  const maybe_expr& get_right_exp() const { return right;}
  const maybe_expr& get_left_exp() const { return left;}
private:
  maybe_expr left;
  maybe_expr right;
};

// Tag definitions
namespace ast_tags
{
  static constexpr inline tag<literal_> literal = {};
  static constexpr inline tag<identifier_> identifier = {};
  static constexpr inline tag<loop_> loop = {};
  static constexpr inline tag<error_> error = {};
  static constexpr inline tag<block_> block = {};
  static constexpr inline tag<assign_> assign = {};
  static constexpr inline tag<binary_exp_> binary_exp = {};
}

