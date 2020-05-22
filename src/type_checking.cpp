#include <type_checking.hpp>
#include <types.hpp>

bool hx_ir_type_checking::check(symbol_map<std::uint_fast32_t>& ctx,
    hx_per_statement_ir& term, std::size_t at, std::uint_fast32_t to_check)
{
  switch(term.kinds[at])
  {
  case IRNodeKind::Type:       return to_check == typtab.Kind_sort_idx;
  case IRNodeKind::Prop:       return to_check == typtab.Type_sort_idx;
  }

  assert(false && "Unhandled type in synthesize.");
  return false;
}

std::uint_fast32_t hx_ir_type_checking::synthesize(symbol_map<std::uint_fast32_t>& ctx,
      hx_per_statement_ir& term, std::size_t at)
{
  switch(term.kinds[at])
  {
  case IRNodeKind::Type:       return typtab.Kind_sort_idx;
  case IRNodeKind::identifier: {
    if(auto it = ctx.find(term.data[at].symb); it != ctx.end())
      return it->second;
    return static_cast<std::uint_fast32_t>(-1); // <- TODO: emit diagnostic
  } break;
  case IRNodeKind::unit:       return typtab.Unit_idx;
                               /*
  case IRNodeKind::app:        {
      auto C = synthesize(ctx, term, at + 1);
    } break;
    */
  case IRNodeKind::assign_type:{
      assert(term.data[at].type_annot != static_cast<std::uint_fast32_t>(-1));
      return term.data[at].type_annot;
    } break;
  case IRNodeKind::assign_data:{
      assert(term.data[at].type_annot != static_cast<std::uint_fast32_t>(-1));
      return term.data[at].type_annot;
    } break;
  }

  assert(false && "Unhandled type in synthesize.");
  return static_cast<std::uint_fast32_t>(-1);
}

