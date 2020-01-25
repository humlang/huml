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
struct assign;

// expressions
struct binary_exp;

using ast_type = std::variant<std::monostate,
        rec_wrap_t<literal>,
        rec_wrap_t<identifier>,
        rec_wrap_t<loop>,
        rec_wrap_t<binary_exp>,
        rec_wrap_t<error>,
        rec_wrap_t<block>,
        rec_wrap_t<assign>
>;

using stmt_type = std::variant<std::monostate,
        rec_wrap_t<loop>,
        rec_wrap_t<error>,
        rec_wrap_t<block>,
        rec_wrap_t<assign>
>;

using exp_type = std::variant<std::monostate,
        rec_wrap_t<literal>,
        rec_wrap_t<identifier>,
        rec_wrap_t<binary_exp>,
        rec_wrap_t<error>
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
            else if(std::holds_alternative<rec_wrap_t<assign>>(o))
              return ast_type(std::move(std::get<rec_wrap_t<assign>>(o)));
            std::exit(2);
        },
};

inline static constexpr auto exp_type_to_ast_type = base_visitor {
        [](exp_type&& o)
        {
            if(std::holds_alternative<rec_wrap_t<literal>>(o))
              return ast_type(std::move(std::get<rec_wrap_t<literal>>(o)));
            else if(std::holds_alternative<rec_wrap_t<identifier>>(o))
              return ast_type(std::move(std::get<rec_wrap_t<identifier>>(o)));
            else if(std::holds_alternative<rec_wrap_t<binary_exp>>(o))
              return ast_type(std::move(std::get<rec_wrap_t<binary_exp>>(o)));
            else if(std::holds_alternative<rec_wrap_t<error>>(o))
              return ast_type(std::move(std::get<rec_wrap_t<error>>(o)));
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

struct assign : base<assign>
{
public:
    assign(tag, rec_wrap_t<identifier> variable, token op, exp_type right);

    const rec_wrap_t<identifier>& var() const {return variable;}
    const exp_type& exp() const {return right;}
private:
    rec_wrap_t<identifier> variable;
    exp_type right;
};

struct binary_exp : base<binary_exp> {
public:
    binary_exp(tag, exp_type left, token op, exp_type right); // TODO function that retunrs the binary expression as string
    const exp_type& get_right_exp() const { return right;}
    const exp_type& get_left_exp() const { return left;}
private:
    exp_type left;
    exp_type right;
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

