#pragma once

#include <symbol.hpp>

#include <vector>

struct scope
{
  scope() : parent(), types(), globals() {}

  std::unique_ptr<scope> parent;

  symbol_map<std::vector<std::uint64_t>> types;
  symbol_map<std::uint64_t> globals;
};

