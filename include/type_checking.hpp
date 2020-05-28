#pragma once

#include <ir.hpp>
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
  hx_ir_type_checking(type_table& typtab, hx_ir& nodes)
    : typtab(typtab), ir(nodes)
  {  }

  std::uint_fast32_t cleanup(std::uint_fast32_t typ);

  bool check(typing_context& ctx,
      std::size_t stmt, std::size_t at, std::uint_fast32_t to_check);

  std::uint_fast32_t synthesize(typing_context& ctx,
      std::size_t stmt, std::size_t at);

  std::uint_fast32_t elim_existentials(typing_context& ctx, std::uint_fast32_t type);

  bool is_instantiation_L(typing_context& ctx, std::uint_fast32_t ealpha, std::uint_fast32_t A);
  bool is_instantiation_R(typing_context& ctx, std::uint_fast32_t A, std::uint_fast32_t ealpha);

  bool is_subtype(typing_context& ctx, std::uint_fast32_t A, std::uint_fast32_t B);

  std::uint_fast32_t eta_synthesis(typing_context& ctx,
      std::size_t stmt, std::size_t at, std::uint_fast32_t type);

  std::uint_fast32_t subst(typing_context& ctx, std::uint_fast32_t type);
  bool is_well_formed(typing_context& ctx, const CTXElement& type);

  type_table& typtab;
  hx_ir& ir;
};

