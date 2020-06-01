#include <type_checking.hpp>

std::pair<bool, std::uint_fast32_t> is_free_variable(hx_ast& tab, std::uint_fast32_t type, std::uint_fast32_t free)
{
  return { false, type };
}


bool hx_ast_type_checking::check(typing_context& ctx, std::size_t at, std::uint_fast32_t to_check)
{
  return false;
}

std::uint_fast32_t hx_ast_type_checking::synthesize(typing_context& ctx, std::size_t at)
{
  return static_cast<std::uint_fast32_t>(-1);
}

bool hx_ast_type_checking::is_subtype(typing_context& ctx, std::uint_fast32_t A, std::uint_fast32_t B)
{
  return false;
}


// Types are not well formed if they contain free variables
bool hx_ast_type_checking::is_well_formed(typing_context& ctx, const CTXElement& ctx_type)
{
  return false;
}

