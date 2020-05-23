#include <type_checking.hpp>
#include <types.hpp>

bool hx_ir_type_checking::check(typing_context& ctx,
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

std::uint_fast32_t hx_ir_type_checking::synthesize(typing_context& ctx,
      hx_per_statement_ir& term, std::size_t at)
{
  switch(term.kinds[at])
  {
  case IRNodeKind::Type:       return typtab.Kind_sort_idx;
  case IRNodeKind::identifier: {
    if(auto it = std::find_if(ctx.begin(), ctx.end(),
                  [at](auto& elem) {
                    if(!std::holds_alternative<id_ref>(elem.data))
                      return false;
                  return std::get<id_ref>(elem.data).id == at; });
        it != ctx.end())
      return std::get<id_ref>(it->data).types_ref; // <- TODO: if existential, convert to actual type
    return static_cast<std::uint_fast32_t>(-1); // <- TODO: emit diagnostic
  } break;
  case IRNodeKind::unit:       return typtab.Unit_idx;
                               /*
  case IRNodeKind::app:        {
      auto C = synthesize(ctx, term, at + 1);
    } break;
    */
  case IRNodeKind::lambda:     {

    } break;
  case IRNodeKind::expr_stmt:  {
      return synthesize(ctx, term, at + 1);
    } break;
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


// Types are not well formed if they contain free variables
bool hx_ir_type_checking::is_well_formed(typing_context& ctx, const CTXElement& ctx_type)
{
  assert(!std::holds_alternative<marker>(ctx_type.data) && "Marker is not a type.");

  if(std::holds_alternative<pseudo_existential*>(ctx_type.data))
  {
    return std::find_if(ctx.begin(), ctx.end(),
              [ex = std::get<pseudo_existential*>(ctx_type.data)](auto& elem)
              {
                if(!std::holds_alternative<pseudo_existential*>(elem.data))
                  return false;
                return ex->num == std::get<pseudo_existential*>(elem.data)->num;
              }) != ctx.end();
  }
  assert((std::holds_alternative<id_ref>(ctx_type.data)));
  auto id_with_type = std::get<id_ref>(ctx_type.data);

  if(id_with_type.references_existential())
  {
    // check if existential is in context

    return std::find_if(ctx.begin(), ctx.end(),
        [ex = id_with_type.existentials_ref](auto& elem)
        {
          if(!std::holds_alternative<id_ref>(elem.data))
            return false;
          auto& e = std::get<id_ref>(elem.data);
          return e.references_existential() && e.existentials_ref == ex;
        }) != ctx.end();
  }

  assert(id_with_type.references_type());
  auto type = id_with_type.types_ref;
  if(type <= typtab.Unit_idx)
    return true;
  switch(typtab.kinds[type])
  {
  default: assert(false && "Cannot enter this"); return false;

  case type_kind::TypeConstructor:
  case type_kind::Id: return std::find_if(ctx.begin(), ctx.end(),
                          [type](auto& elem) {
                            if(!std::holds_alternative<id_ref>(elem.data))
                              return false;
                            auto& e = std::get<id_ref>(elem.data);

                            return e.references_type() && e.types_ref == type;
                      }) != ctx.end();

  case type_kind::Application: {
    bool first  = is_well_formed(ctx, CTXElement { id_ref { id_with_type.id, typtab.data[type].args.front() } });
    bool second = is_well_formed(ctx, CTXElement { id_ref { id_with_type.id, typtab.data[type].args.back() } });

    return first && second;
  };

  case type_kind::Pi: {
      ctx.emplace_back(CTXElement { id_ref { id_with_type.id, typtab.data[type].args.front() } });
      bool well_formed = is_well_formed(ctx, CTXElement { id_ref { id_with_type.id, typtab.data[type].args.back() } });
      ctx.pop_back();

      return well_formed;
    };
  }
}

