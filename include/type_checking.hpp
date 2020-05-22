#pragma once

#include <per_statement_ir.hpp>
#include <symbol.hpp>

struct type_table;
struct type_base;

struct hx_ir_type_checking
{
  hx_ir_type_checking(type_table& typtab)
    : typtab(typtab)
  {  }

  bool check(symbol_map<std::shared_ptr<type_base>>& ctx,
      hx_per_statement_ir& term, std::size_t at, const type_base* to_check);

  std::shared_ptr<type_base> synthesize(symbol_map<std::shared_ptr<type_base>>& ctx,
      hx_per_statement_ir& term, std::size_t at);


  type_table& typtab;
};

