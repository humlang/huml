#include <type_checking.hpp>

#if 0

std::uint_fast32_t hx_ir_type_checking::cleanup(std::uint_fast32_t at)
{
  return 0;
}

std::pair<bool, std::uint_fast32_t> is_free_variable(hx_ir& tab, std::uint_fast32_t type, std::uint_fast32_t free)
{
  if(type == free)
    return { true, type }; // free occurs in type, i.e. it IS type

  std::size_t iter = type;
  for(std::size_t i = 0; i < tab.data[type].argc; ++i)
  {
    auto [a, b] = is_free_variable(tab, iter + 1, free);
    if(a)
      return { true, type };
    iter = b;
  }
  return { false, type };
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

#endif

