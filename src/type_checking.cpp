#include <type_checking.hpp>
#include <types.hpp>

/*
bool hx_ir_type_checking::check(symbol_map<const type_base*>& ctx,
    hx_per_statement_ir& term, std::size_t at, const type_base* to_check)
{

}

const type_base* hx_ir_type_checking::synthesize(symbol_map<const type_base*>& ctx,
      hx_per_statement_ir& term, std::size_t at)
{
  switch(term.kinds[at])
  {
  case IRNodeKind::Type:       return typtab[typtab.Kind_sort_idx];
  case IRNodeKind::identifier: {
    if(auto it = ctx.find(term.data[at].symb); it != ctx.end())
      return it->second;
    return nullptr;
  } break;
  case IRNodeKind::unit:       return typtab[typtab.Unit_idx];
  case IRNodeKind::app:        {
      auto C = synthesize(ctx, term, at + 1);
    } break;
  }

  assert(false && "Unhandled type in synthesize.");
  return nullptr;
}

*/
