#pragma once

#include "source_range.hpp"
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

struct error_;             using error = rec_wrap_t<error_>;

// statements
struct assign_;            using assign = rec_wrap_t<assign_>;
struct assign_type_;       using assign_type = rec_wrap_t<assign_type_>;
struct expr_stmt_;         using expr_stmt = rec_wrap_t<expr_stmt_>;

// expressions
struct binary_exp_;        using binary_exp = rec_wrap_t<binary_exp_>;
struct identifier_;        using identifier = rec_wrap_t<identifier_>;
struct literal_;           using literal = rec_wrap_t<literal_>;
struct block_;             using block = rec_wrap_t<block_>;
struct unit_;              using unit = rec_wrap_t<unit_>;
struct tuple_;             using tuple = rec_wrap_t<tuple_>;
struct bot_;               using bot = rec_wrap_t<bot_>;
struct top_;               using top = rec_wrap_t<top_>;
struct app_;               using app = rec_wrap_t<app_>;
struct access_;            using access = rec_wrap_t<access_>;
struct lambda_;            using lambda = rec_wrap_t<lambda_>;
struct pattern_matcher_;   using pattern_matcher = rec_wrap_t<pattern_matcher_>;

// pattern match arm
struct match_;             using match = rec_wrap_t<match_>;

// pattern
struct pattern_;           using pattern = rec_wrap_t<pattern_>;

using stmt_type = std::variant<std::monostate,
        assign,
        expr_stmt,
        assign_type
>;

using exp_type = std::variant<std::monostate,
        unit,
        tuple,
        literal,
        identifier,
        binary_exp,
        block,
        top,
        bot,
        app,
        access,
        lambda,
        pattern_matcher
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
using maybe_match = std::variant<std::monostate, match, rec_wrap_t<error_>>;
using maybe_patt = std::variant<std::monostate, pattern, tuple, identifier, literal,
                                                unit, app, rec_wrap_t<error_>>;

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

struct unit_ : base<unit_>
{
public:
  unit_(tag, token tok);
};

struct top_ : base<top_>
{
public:
  top_(tag, token tok);
};

struct bot_ : base<bot_>
{
public:
  bot_(tag, token tok);
};

struct app_ : base<app_>
{
public:
  app_(tag, maybe_expr f, maybe_expr arg);

  const maybe_expr& fun() const { return f; }
  const maybe_expr& argument() const { return arg; }
private:
  maybe_expr f;
  maybe_expr arg;
};

struct access_ : base<access_>
{
public:
  access_(tag, token tok, maybe_expr tup, maybe_expr at);

  const maybe_expr& tuple() const { return tup; }
  const maybe_expr& accessed_at() const { return at; }
private:
  maybe_expr tup;
  maybe_expr at;
};

struct tuple_ : base<tuple_>
{
public:
  tuple_(tag, token tok, std::vector<maybe_expr> args);

  const std::vector<maybe_expr>& expressions() const { return exprs; }
private:
  std::vector<maybe_expr> exprs;
};

struct lambda_ : base<lambda_>
{
public:
  lambda_(tag, token tok, identifier arg, maybe_expr body);

  const identifier& argument() const { return arg; }
  const maybe_expr& fbody() const { return body; }
private:
  identifier arg;
  maybe_expr body;
};

struct pattern_ : base<pattern_>
{
public:
  pattern_(tag, token tok, maybe_patt patt);

  const maybe_patt& pattern() const { return patt; }
private:
  maybe_patt patt;
};

struct match_ : base<match_>
{
public:
  match_(tag, token tok, maybe_patt pat, maybe_expr expr);

  const maybe_patt& pattern() const { return pat; }
  const maybe_expr& expression() const { return expr; }
private:
  maybe_patt pat;
  maybe_expr expr;
};

struct pattern_matcher_ : base<pattern_matcher_>
{
public:
  pattern_matcher_(tag, token tok, maybe_expr to_match, std::vector<maybe_match> patterns);

  const maybe_expr& to_be_matched() const { return to_match; }
  const std::vector<maybe_match>& match_patterns() const { return patterns; }
private:
  maybe_expr to_match;
  std::vector<maybe_match> patterns;
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

struct block_ : base<block_>
{
public:
  block_(tag, token tok, std::vector<maybe_expr> exprs);

  const std::vector<maybe_expr>& expressions() const { return exprs; }
private:
  std::vector<maybe_expr> exprs;
};

struct assign_ : base<assign_>
{
public:
  assign_(tag, maybe_expr variable, token op, maybe_expr right);

  const maybe_expr& var() const { return variable; }
  const maybe_expr& exp() const { return right; }
private:
  maybe_expr variable;
  maybe_expr right;
};

struct assign_type_ : base<assign_type_>
{
public:
  assign_type_(tag, token tok, maybe_expr variable, std::vector<maybe_expr> constructors);

  const maybe_expr& name() const { return variable; }
  const std::vector<maybe_expr>& constructors() const { return ctors; }
private:
  maybe_expr variable;
  std::vector<maybe_expr> ctors;
};

struct expr_stmt_ : base<expr_stmt_>
{
public:
  expr_stmt_(tag, maybe_expr expr);

  const maybe_expr& exp() const { return expr; }
private:
  maybe_expr expr;
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
  static constexpr inline tag<error_> error = {};
  static constexpr inline tag<block_> block = {};
  static constexpr inline tag<assign_> assign = {};
  static constexpr inline tag<expr_stmt_> expr_stmt = {};
  static constexpr inline tag<binary_exp_> binary_exp = {};
  static constexpr inline tag<unit_> unit = {};
  static constexpr inline tag<tuple_> tuple = {};
  static constexpr inline tag<top_> top = {};
  static constexpr inline tag<bot_> bot = {};
  static constexpr inline tag<pattern_> pattern = {};
  static constexpr inline tag<match_> match = {};
  static constexpr inline tag<pattern_matcher_> pattern_matcher = {};
  static constexpr inline tag<app_> app = {};
  static constexpr inline tag<access_> access = {};
  static constexpr inline tag<lambda_> lambda = {};
  static constexpr inline tag<assign_type_> assign_type = {};
}

inline static constexpr auto ast_get_loc_helper = base_visitor {
  // always add the following four edge cases
  [](auto&& rec, const std::monostate& t) -> source_range
  { assert(false && "Should never be called."); return source_range{}; },

  [](auto&& rec, auto& arg) -> source_range {
    return arg->loc();
  },
  [](auto&& rec, const stmt_type& typ) -> source_range {
    return std::visit(rec, typ);
  },
  [](auto&& rec, const exp_type& typ) -> source_range {
    return std::visit(rec, typ);
  }
};

inline static constexpr auto ast_get_loc = 
			[](const auto& arg) -> source_range
      {
        struct state
        {  };

        auto rec = stateful_recursor(state{  }, ast_get_loc_helper);

        return ast_get_loc_helper(rec, arg);
      };



