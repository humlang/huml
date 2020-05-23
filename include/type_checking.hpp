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

struct id_ref
{
  static constexpr std::uint_fast32_t no_ref = static_cast<std::uint_fast32_t>(-1);

  bool references_type() const        { return types_ref != no_ref; }
  bool references_existential() const { return existentials_ref != no_ref; }

  std::uint_fast32_t id;

  std::uint_fast32_t types_ref { no_ref };
  std::uint_fast32_t existentials_ref { no_ref };
};

struct CTXElement
{
  std::variant<id_ref, pseudo_existential*, marker> data;
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


  std::vector<pseudo_existential> existentials;

  type_table& typtab;
};

