#pragma once

#include <token.hpp>
#include <cstdint>
#include <vector>

struct type; 

enum class IRNodeKind : std::int_fast8_t
{
  undef = 0,
  literal,
  unit,
  top,
  bot,
  app,
  access,
  tuple,
  lambda,
  param,
  pattern,
  match,
  pattern_matcher,
  identifier,
  block,
  assign,
  assign_type,
  expr_stmt,
  binary_exp,
  type_check,
  pi
};

struct IRData
{
  symbol symb;
  std::uint_fast32_t typ_ref { static_cast<std::uint_fast32_t>(-1) }; //<- to be inferred

  std::vector<std::uint_fast32_t> additional_references; // Stores an explicit position in the vector
};

struct hx_per_statement_ir
{
  std::vector<IRNodeKind> kinds;
  std::vector<std::uint_fast32_t> references; // stores the number of arguments per kind
  std::vector<IRData> data;

  struct free_variable
  {
    std::uint_fast32_t potentially_bounded_ref { static_cast<std::uint_fast32_t>(-1) };
    std::vector<std::uint_fast32_t> refs;
  };
  symbol_map<free_variable> free_variable_roots;

  std::shared_ptr<symbol> node_name; // used for assign or type assign, otherwise it's the empty symbol


  std::vector<std::shared_ptr<type>> types;

  void print(std::ostream& os) const
  {
    assert(!kinds.empty() && "IR cannot be zero, there is never an empty module!");
    print_subnode(os, 0);
  }
  std::uint_fast32_t print_subnode(std::ostream& os, std::uint_fast32_t node) const;
};

