#pragma once

#include <token.hpp>
#include <cstdint>
#include <vector>

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
  error,
  block,
  assign,
  assign_type,
  expr_stmt,
  binary_exp
};

struct IRData
{
  token tok;
};

struct hx_ir
{
  std::vector<IRNodeKind> kinds;
  std::vector<std::int_fast32_t> references; // stores the number of arguments per kind
  std::vector<IRData> data;
};

