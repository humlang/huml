#pragma once

#include "source_range.hpp"

#include <ast_fwd.hpp>

// Tags are needed to make variant construction unique
namespace ast_tags
{
  template<typename T>
  struct tag
  {
    // can be convenient.
    template<typename... Args>
    static std::size_t make_node(aligned_ast_vec& v, Args&&... args)
    { v.emplace_back<T>(tag<T>(), std::forward<Args>(args)...); return v.back<T>().id(); }
  };
}

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
  app_(tag, std::size_t f, std::size_t arg);

  std::size_t fun() const { return f; }
  std::size_t argument() const { return arg; }
private:
  std::size_t f;
  std::size_t arg;
};

struct access_ : base<access_>
{
public:
  access_(tag, token tok, std::size_t tup, std::size_t at);

  std::size_t tuple() const { return tup; }
  std::size_t accessed_at() const { return at; }
private:
  std::size_t tup;
  std::size_t at;
};

struct tuple_ : base<tuple_>
{
public:
  tuple_(tag, token tok, std::vector<std::size_t> args);

  const std::vector<std::size_t>& expressions() const { return exprs; }
private:
  std::vector<std::size_t> exprs;
};

struct lambda_ : base<lambda_>
{
public:
  lambda_(tag, token tok, std::size_t arg, std::size_t body);

  std::size_t argument() const { return arg; }
  std::size_t fbody() const { return body; }
private:
  std::size_t arg;
  std::size_t body;
};

struct pattern_ : base<pattern_>
{
public:
  pattern_(tag, token tok, std::size_t patt);

  std::size_t pattern() const { return patt; }
private:
  std::size_t patt;
};

struct match_ : base<match_>
{
public:
  match_(tag, token tok, std::size_t pat, std::size_t expr);

  std::size_t pattern() const { return pat; }
  std::size_t expression() const { return expr; }
private:
  std::size_t pat;
  std::size_t expr;
};

struct pattern_matcher_ : base<pattern_matcher_>
{
public:
  pattern_matcher_(tag, token tok, std::size_t to_match, std::vector<std::size_t> patterns);

  std::size_t to_be_matched() const { return to_match; }
  const std::vector<std::size_t>& match_patterns() const { return patterns; }
private:
  std::size_t to_match;
  std::vector<std::size_t> patterns;
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
  block_(tag, token tok, std::vector<std::size_t> exprs);

  const std::vector<std::size_t>& expressions() const { return exprs; }
private:
  std::vector<std::size_t> exprs;
};

struct assign_ : base<assign_>
{
public:
  assign_(tag, std::size_t variable, token op, std::size_t right);

  std::size_t var() const { return variable; }
  std::size_t exp() const { return right; }
private:
  std::size_t variable;
  std::size_t right;
};

struct assign_type_ : base<assign_type_>
{
public:
  assign_type_(tag, token tok, std::size_t variable, std::vector<std::size_t> constructors);

  std::size_t name() const { return variable; }
  const std::vector<std::size_t>& constructors() const { return ctors; }
private:
  std::size_t variable;
  std::vector<std::size_t> ctors;
};

struct expr_stmt_ : base<expr_stmt_>
{
public:
  expr_stmt_(tag, std::size_t expr);

  std::size_t exp() const { return expr; }
private:
  std::size_t expr;
};

struct binary_exp_ : base<binary_exp_>
{
public:
  binary_exp_(tag, std::size_t left, token op, std::size_t right);

  std::size_t get_right_exp() const { return right;}
  std::size_t get_left_exp() const { return left;}
private:
  std::size_t left;
  std::size_t right;
};

// Tag definitions
namespace ast_tags
{
  static constexpr inline tag<literal_> literal = {  };
  static constexpr inline tag<identifier_> identifier = {  };
  static constexpr inline tag<error_> error = {  };
  static constexpr inline tag<block_> block = {  };
  static constexpr inline tag<assign_> assign = {  };
  static constexpr inline tag<expr_stmt_> expr_stmt = {  };
  static constexpr inline tag<binary_exp_> binary_exp = {  };
  static constexpr inline tag<unit_> unit = {  };
  static constexpr inline tag<tuple_> tuple = {  };
  static constexpr inline tag<top_> top = {  };
  static constexpr inline tag<bot_> bot = {  };
  static constexpr inline tag<pattern_> pattern = {  };
  static constexpr inline tag<match_> match = {  };
  static constexpr inline tag<pattern_matcher_> pattern_matcher = {  };
  static constexpr inline tag<app_> app = {  };
  static constexpr inline tag<access_> access = {  };
  static constexpr inline tag<lambda_> lambda = {  };
  static constexpr inline tag<assign_type_> assign_type = {  };
}



