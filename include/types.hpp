#pragma once

#include <symbol.hpp>
#include <ast_fwd.hpp>

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

  type_kind kind;
  symbol readable_identifier;

  virtual bool is_polymorphic() const { return false; }
  virtual bool is_inductive() const   { return false; }
};

// forall alpha
struct polymorphic : type
{
  bool is_polymorphic() const override { return true; }
};

// type nat = Zero | Succ(n : nat)
struct inductive : type
{
  inductive(symbol readable_identifier, aligned_ast_vec&& ctrs_data, std::vector<std::int_fast32_t>&& ctrs)
    : type(type_kind::monomorphic, readable_identifier), constructors_data(ctrs_data), constructors(ctrs)
  {  }

  aligned_ast_vec constructors_data;
  std::vector<std::int_fast32_t> constructors;

  bool is_inductive() const override { return true; }
};

