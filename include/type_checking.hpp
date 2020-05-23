#pragma once

#include <per_statement_ir.hpp>
#include <symbol.hpp>

#include <variant>

struct type_table;

struct pseudo_existential
{
  static constexpr std::uint_fast32_t no_solution = static_cast<std::uint_fast32_t>(-1);

  std::uint_fast32_t num;
  std::uint_fast32_t solution { no_solution };
};

struct marker
{
  pseudo_existential& ref;
};

struct CTXElement
{
  std::variant<std::uint_fast32_t, pseudo_existential, marker> data;
};

std::vector<CTXElement> typedef typing_context;

struct hx_ir_type_checking
{
  hx_ir_type_checking(type_table& typtab)
    : typtab(typtab)
  {  }

  bool check(typing_context& ctx,
      hx_per_statement_ir& term, std::size_t at, std::uint_fast32_t to_check);

  std::uint_fast32_t synthesize(typing_context& ctx,
      hx_per_statement_ir& term, std::size_t at);


  bool is_well_formed(typing_context& ctx, const CTXElement& type);

  type_table& typtab;
};

