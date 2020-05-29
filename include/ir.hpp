#pragma once

#include <source_range.hpp>
#include <ir_nodes.hpp>
#include <types.hpp>

struct hx_ir
{
  struct edge
  {
    std::uint_fast32_t from;
    std::uint_fast32_t to;

    bool operator<(const edge& other) const
    { return from < other.from && to < other.to; }
  };

  hx_ir(); 

  std::uint_fast32_t add(IRNodeKind kind, IRData&& dat, IRDebugData&& dbg_data);

  bool type_checks();
  void print(std::ostream& os);
  std::uint_fast32_t print_node(std::ostream& os, std::uint_fast32_t node);

  std::vector<IRNodeKind> kinds;
  std::vector<IRData> data;
  std::vector<IRDebugData> dbg_data;

  std::vector<std::uint_fast32_t> roots;
  std::vector<std::uint_fast32_t> types;

  // node -> name of free variable
  tsl::robin_map<std::uint_fast32_t, symbol_set> free_variables_per_node;

  std::vector<edge> edges;
  std::vector<symbol> globally_free_variables;

  type_table typtab;
};

