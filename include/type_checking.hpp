#pragma once

#include <symbol.hpp>
#include <ast.hpp>

struct CTXElement
{
  std::size_t id_def; // <- absolute position of the binding occurence of id
  std::size_t type;   // <- absolute position of a type
};

std::vector<CTXElement> typedef typing_context;

struct hx_ast_type_checking
{
  hx_ast_type_checking(hx_ast& nodes)
    : ast(nodes)
  {  }

  bool check(typing_context& ctx, std::size_t at, std::uint_fast32_t to_check);
  std::uint_fast32_t synthesize(typing_context& ctx, std::size_t at);

  bool is_subtype(typing_context& ctx, std::uint_fast32_t A, std::uint_fast32_t B);

  std::uint_fast32_t subst(typing_context& ctx, std::uint_fast32_t type);
  bool is_well_formed(typing_context& ctx, const CTXElement& type);

  hx_ast& ast;
};

