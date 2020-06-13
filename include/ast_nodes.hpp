#pragma once

#include <source_range.hpp>
#include <symbol.hpp>

enum class ASTNodeKind : std::int_fast8_t
{
  exist = -1, // <- only used for type_checking, must not occur anywhere else
  undef = 0,
  Kind,
  Type,
  Prop,
  unit,
  app,
  lambda,
  match,
  pattern_matcher,
  identifier,
  assign,
  assign_type,
  assign_data,
  expr_stmt,
  directive,
  map_impl,
};

struct ASTData
{
  constexpr static std::uint_fast32_t no_type = static_cast<std::uint_fast32_t>(-1);
  constexpr static std::int_fast32_t no_ref   = std::numeric_limits<std::uint_fast32_t>::max();

  symbol name;

  std::size_t argc { 0 };

  std::uint_fast32_t type_annot { no_type };

  std::int_fast32_t back_ref { no_ref };
};

struct ASTDebugData
{
  source_range loc;
};

