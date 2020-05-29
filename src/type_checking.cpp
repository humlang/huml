#include <type_checking.hpp>
#include <types.hpp>

std::uint_fast32_t hx_ir_type_checking::cleanup(std::uint_fast32_t at)
{
  return 0;
}

bool is_free_variable(type_table& tab, std::uint_fast32_t type, std::uint_fast32_t free)
{
  if(type == free)
    return true; // free occurs in type, i.e. it IS type

  for(auto& x : tab.data[type].args)
    if(is_free_variable(tab, x, free))
      return true;
  return false;
}


bool hx_ir_type_checking::check(typing_context& ctx,
    std::size_t stmt, std::size_t at, std::uint_fast32_t to_check)
{
  return 0;
}

std::uint_fast32_t hx_ir_type_checking::synthesize(typing_context& ctx,
      std::size_t stmt, std::size_t at)
{
  return 0;
}

bool hx_ir_type_checking::is_instantiation_L(typing_context& ctx, std::uint_fast32_t ealpha, std::uint_fast32_t A)
{
  return false;
}

bool hx_ir_type_checking::is_instantiation_R(typing_context& ctx, std::uint_fast32_t A, std::uint_fast32_t ealpha)
{
  return false;
}

bool hx_ir_type_checking::is_subtype(typing_context& ctx, std::uint_fast32_t A, std::uint_fast32_t B)
{
  return false;
}


std::uint_fast32_t hx_ir_type_checking::eta_synthesis(typing_context& ctx, std::size_t stmt, std::size_t at, std::uint_fast32_t type)
{
  return false;
}

// Types are not well formed if they contain free variables
bool hx_ir_type_checking::is_well_formed(typing_context& ctx, const CTXElement& ctx_type)
{
  return false;
}

