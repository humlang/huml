#pragma once

#include <per_statement_ir.hpp>
#include <symbol.hpp>

struct type_table;

struct hx_ir_type_checking
{
  hx_ir_type_checking(type_table& typtab)
    : typtab(typtab)
  {  }

  bool check(symbol_map<std::uint_fast32_t>& ctx,
      hx_per_statement_ir& term, std::size_t at, std::uint_fast32_t to_check);

  std::uint_fast32_t synthesize(symbol_map<std::uint_fast32_t>& ctx,
      hx_per_statement_ir& term, std::size_t at);


  type_table& typtab;
};

