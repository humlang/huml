#pragma once

#include <source_range.hpp>
#include <ir_nodes.hpp>

struct hx_ir
{
  struct data_constructors
  {
    std::vector<IRData> data;
  };
  struct edge
  {
    std::uint_fast32_t from;
    std::uint_fast32_t to;

    bool operator<(const edge& other) const
    { return from < other.from && to < other.to; }
  };
  static constexpr std::size_t Kind_sort_idx = 0;
  static constexpr std::size_t Type_sort_idx = 1;
  static constexpr std::size_t Prop_sort_idx = 2;
  static constexpr std::size_t Unit_idx = 3;

  hx_ir(); 

  std::uint_fast32_t add(IRNodeKind kind, IRData&& dat, IRDebugData&& dbg_data);

  template<typename Iterator>
  std::uint_fast32_t add(Iterator insert_at, IRNodeKind kind, IRData&& dat, IRDebugData&& dbg_data);

  std::uint_fast32_t subst(std::uint_fast32_t in, std::uint_fast32_t what, std::uint_fast32_t with);

  bool type_checks();
  void print(std::ostream& os);
  std::uint_fast32_t print_node(std::ostream& os, std::uint_fast32_t node);

  std::vector<IRNodeKind> kinds;
  std::vector<IRData> data;
  std::vector<IRDebugData> dbg_data;

  std::vector<std::uint_fast32_t> roots;
  std::vector<std::uint_fast32_t> types;
  tsl::robin_map<std::uint_fast32_t, data_constructors> constructors;

  // node -> name of free variable
  tsl::robin_map<std::uint_fast32_t, symbol_set> free_variables_per_node;

  std::vector<edge> edges;
  std::vector<symbol> globally_free_variables;
};

