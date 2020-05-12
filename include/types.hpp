#pragma once

#include <symbol.hpp>

enum class type_kind
{
  polymorphic,
  existential,
  monomorphic
};

struct type
{
  type(type_kind kind, symbol readable_identifier)
    : kind(kind), readable_identifier(readable_identifier)
  {  }

  virtual bool is_polymorphic() const { return false; }
  virtual bool is_inductive() const   { return false; }

  type_kind kind;
  symbol readable_identifier;
};

// forall alpha
struct polymorphic : type
{
  bool is_polymorphic() const override { return true; }
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
  };
  struct identifier : base
  {
    identifier(symbol symb)
      : id(symb)
    {  }

    bool is_identifier() const override { return true; }

    symbol id;
  };
  struct unit : base
  { bool is_unit() const override { return true; } };
  struct app : base
  {
    app(std::shared_ptr<base> caller, std::shared_ptr<base> param)
      : caller(caller), param(param)
    {  }

    bool is_app() const override { return true; }

    std::shared_ptr<base> caller;
    std::shared_ptr<base> param;
  };
  struct tuple : base
  {
    tuple(std::vector<std::shared_ptr<base>> elems)
      : elems(elems)
    {  }

    bool is_tuple() const override { return true; }

    std::vector<std::shared_ptr<base>> elems;
  };
}
struct inductive : type
{
  inductive(symbol readable_identifier, std::vector<std::shared_ptr<constructors::base>>&& ctrs)
    : type(type_kind::monomorphic, readable_identifier), constructors(std::move(ctrs))
  {  }

  bool is_inductive() const override { return true; }

  std::vector<std::shared_ptr<constructors::base>> constructors;
};

