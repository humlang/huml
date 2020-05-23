#pragma once

#include <per_statement_ir.hpp>
#include <symbol.hpp>

#include <variant>

struct type_table;

struct marker
{
  std::uint_fast32_t existential_ref;
};

struct id_or_type_ref
{
  static constexpr std::uint_fast32_t no_ref = static_cast<std::uint_fast32_t>(-1);

  bool references() const { return type != no_ref; }

  std::uint_fast32_t id   { no_ref };
  std::uint_fast32_t type { no_ref };
};

struct CTXElement
{
  std::variant<id_or_type_ref, marker> data;
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

