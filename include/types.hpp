#pragma once

#include <per_statement_ir.hpp>
#include <symbol.hpp>

struct type
{
  virtual bool is_inductive() const { return false; }
};

// we have dependend types, so we need a way to specify arbitrary expressions
// such as "\x. x /\ ~x" or "int 32"
struct ir_based_type
{
  hx_per_statement_ir type;
};

// Π(x : A). B       where B might use x. Example: Π(n : nat). Vec n
struct pi_type : type
{
  pi_type(symbol binder, std::shared_ptr<type> A, std::shared_ptr<type> B)
    : type(), binder(binder), A(A), B(B)
  {  }

  symbol binder;

  std::shared_ptr<type> A;
  std::shared_ptr<type> B;
};

// type nat = Zero | Succ(n : nat)
namespace constructors
{
  struct base
  {
    virtual bool is_identifier() const { return false; }   
    virtual bool is_unit() const { return false; }
    virtual bool is_app() const { return false; }
    virtual bool is_tuple() const { return false; }

    virtual symbol name() const = 0;
  };
  struct identifier : base
  {
    identifier(symbol symb)
      : id(symb)
    {  }

    bool is_identifier() const override { return true; }

    symbol name() const override { return id; }

    symbol id;
  };
  struct unit : base
  {
    bool is_unit() const override { return true; }
    symbol name() const override { assert(false && "ctor arg has no name"); return ""; }
  };
  struct app : base
  {
    app(std::shared_ptr<base> caller, std::shared_ptr<base> param)
      : caller(caller), param(param)
    {  }

    bool is_app() const override { return true; }

    symbol name() const override { return caller->name(); }

    std::shared_ptr<base> caller;
    std::shared_ptr<base> param;
  };
  struct tuple : base
  {
    tuple(std::vector<std::shared_ptr<base>> elems)
      : elems(elems)
    {  }

    bool is_tuple() const override { return true; }

    symbol name() const override { assert(false && "ctor arg has no name"); return ""; }

    std::vector<std::shared_ptr<base>> elems;
  };
}
struct inductive : type
{
  inductive(symbol readable_identifier, std::vector<std::shared_ptr<constructors::base>>&& ctrs)
    : name(readable_identifier), constructors(std::move(ctrs))
  {  }

  bool is_inductive() const { return true; }

  symbol name;
  std::vector<std::shared_ptr<constructors::base>> constructors;
};

