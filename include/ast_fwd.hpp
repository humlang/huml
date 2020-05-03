#pragma once

#include <arena.hpp>
#include <token.hpp>

#include <cstdint>
#include <variant>
#include <atomic>
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
using rec_wrap_t = T*;

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
      pattern, match,
      error
>;
using aligned_ast_vec = aligned_variant_vec<assign_, expr_stmt_, assign_type_, unit_, tuple_, literal_, identifier_,
      binary_exp_, block_, top_, bot_, app_, access_, lambda_, pattern_matcher_, pattern_, match_, error_>;
// The error node is special, as it can also be an expression or a statement.
// To implement this in a somewhat clean way (visitors will do fine with this)
// we introduce "optional" types, where the content is either a exp/stmt or an error
//  -> This forces us to really handle errors somewhat correctly in the parser
using maybe_expr = std::variant<std::monostate, exp_type, rec_wrap_t<error_>>;
using maybe_stmt = std::variant<std::monostate, stmt_type, rec_wrap_t<error_>>;
using maybe_match = std::variant<std::monostate, match, rec_wrap_t<error_>>;
using maybe_patt = std::variant<std::monostate, pattern, tuple, identifier, literal,
                                                unit, app, rec_wrap_t<error_>>;

