#include <type_checking.hpp>
#include <types.hpp>

bool hx_ir_type_checking::check(typing_context& ctx,
    hx_per_statement_ir& term, std::size_t at, std::uint_fast32_t to_check)
{
  switch(term.kinds[at])
  {
  // T-Type<=
  case IRNodeKind::Type:       return to_check == typtab.Kind_sort_idx;
  // T-Prop<=
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
  // T-Type=>
  case IRNodeKind::Type:       return typtab.Kind_sort_idx; 
  // T-Prop=>
  case IRNodeKind::Prop:       return typtab.Type_sort_idx;
  // T-Var=>
  case IRNodeKind::identifier: {
    if(auto it = std::find_if(ctx.begin(), ctx.end(),
                  [at](auto& elem) {
                    if(!std::holds_alternative<id_or_type_ref>(elem.data))
                      return false;
                  return std::get<id_or_type_ref>(elem.data).id == at; });
        it != ctx.end())
      return std::get<id_or_type_ref>(it->data).type;
    return static_cast<std::uint_fast32_t>(-1); // <- TODO: emit diagnostic
  } break;
  // T-1=>
  case IRNodeKind::unit:       return typtab.Unit_idx;
                               /*
  case IRNodeKind::app:        {
      auto C = synthesize(ctx, term, at + 1);
    } break;
    */
  // T-I=>
  case IRNodeKind::lambda:     {
      // alpha
      std::uint_fast32_t alpha = typtab.kinds.size();
      typtab.kinds.emplace_back(type_kind::TypeCheckExistential);
      typtab.data.emplace_back(TypeData { symbol("alpha_" + std::to_string(alpha)) });

      // beta
      std::uint_fast32_t beta  = typtab.kinds.size();
      typtab.kinds.emplace_back(type_kind::TypeCheckExistential);
      typtab.data.emplace_back(TypeData { symbol("beta_" + std::to_string(beta)) });

      ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha } });
      ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, beta  } });
      ctx.emplace_back(CTXElement { id_or_type_ref { at + 1, alpha } });

      if(!check(ctx, term, at + 2, beta))
        return static_cast<std::uint_fast32_t>(-1);
      
      ctx.pop_back();

      // `alpha -> beta`
      type_tags::pi.make_node(typtab, TypeData { "", { alpha, beta } });
    } break;
  // T-Anno
  case IRNodeKind::type_check: {
      if(!is_well_formed(ctx, CTXElement { id_or_type_ref { at, term.data[at].type_annot } }))
        return static_cast<std::uint_fast32_t>(-1); // <- TODO: emit diagnostic
      if(!check(ctx, term, at, term.data[at].type_annot))
        return static_cast<std::uint_fast32_t>(-1); // <- TODO: emit diagnostic
      return term.data[at].type_annot;
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

  auto id_with_type = std::get<id_or_type_ref>(ctx_type.data);

  if(typtab.kinds[id_with_type.type] == type_kind::TypeCheckExistential)
  {
    // check if existential is in context

    return std::find_if(ctx.begin(), ctx.end(),
        [ex = id_with_type.type](auto& elem)
        {
          if(!std::holds_alternative<id_or_type_ref>(elem.data))
            return false;
          auto& e = std::get<id_or_type_ref>(elem.data);
          return e.references() && e.type == ex;
        }) != ctx.end();
  }

  auto type = id_with_type.type;
  if(type <= typtab.Unit_idx)
    return true;
  switch(typtab.kinds[type])
  {
  default: assert(false && "Cannot enter this"); return false;

  case type_kind::TypeConstructor:
  case type_kind::Id: return std::find_if(ctx.begin(), ctx.end(),
                          [type, id = id_with_type.id](auto& elem) {
                            if(!std::holds_alternative<id_or_type_ref>(elem.data))
                              return false;
                            auto& e = std::get<id_or_type_ref>(elem.data);

                            return e.id == id && e.references() && e.type == type;
                      }) != ctx.end();

  case type_kind::Application: {
    bool first  = is_well_formed(ctx, CTXElement { id_or_type_ref { id_with_type.id,
                                                    typtab.data[type].args.front() } });
    bool second = is_well_formed(ctx, CTXElement { id_or_type_ref { id_with_type.id,
                                                    typtab.data[type].args.back() } });

    return first && second;
  };

  case type_kind::Pi: {
      ctx.emplace_back(CTXElement { id_or_type_ref { id_with_type.id,
                                     typtab.data[type].args.front() } });
      bool well_formed = is_well_formed(ctx, CTXElement { id_or_type_ref { id_with_type.id,
                                                            typtab.data[type].args.back() } });
      ctx.pop_back();

      return well_formed;
    };
  }
}

