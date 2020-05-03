#pragma once

#include <symbol.hpp>
#include <ast.hpp>

#include <vector>

struct scope
{
  scope() : parent(), globals() {}

  std::unique_ptr<scope> parent;

  symbol_map<std::vector<std::size_t>> types;
  symbol_map<std::size_t> globals;

  std::vector<std::size_t> roots;
  aligned_ast_vec ast_storage;
};

