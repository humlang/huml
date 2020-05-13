#pragma once

#include <per_statement_ir.hpp>

struct hx_ir
{
  struct edge
  {
    std::uint_fast32_t from;
    std::uint_fast32_t to;

    bool operator<(const edge& other) const
    { return from < other.from && to < other.to; }
  };

  hx_ir(std::vector<hx_per_statement_ir>&& nodes); 

  std::vector<hx_per_statement_ir> nodes;
  std::vector<edge> edges;

  std::vector<symbol> globally_free_variables;
};

