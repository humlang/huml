#include <type_checking.hpp>
#include <types.hpp>

bool is_free_variable(type_table& tab, std::uint_fast32_t type, std::uint_fast32_t free)
{
  assert(tab.kinds[free] == type_kind::TypeCheckExistential && "Free variable check only works for existentials a.t.m.");

  if(type == free) // <- TODO: equality is a bit more sophisticated!
    return false;

  for(auto& x : tab.data[type].args)
    if(!is_free_variable(tab, type, x))
      return false;
  return true;
}


bool hx_ir_type_checking::check(typing_context& ctx,
    hx_per_statement_ir& term, std::size_t at, std::uint_fast32_t to_check)
{
  switch(term.kinds[at])
  {
  // T-Type<=
  case IRNodeKind::Type:       return to_check == typtab.Kind_sort_idx;
  // T-Prop<=
  case IRNodeKind::Prop:       return to_check == typtab.Type_sort_idx;
  // T-I<=
  case IRNodeKind::lambda:     {
      if(typtab.kinds[to_check] != type_kind::Pi)
        return static_cast<std::uint_fast32_t>(-1); // <- TODO: emit diagnostic

      ctx.emplace_back(CTXElement { id_or_type_ref { at + 1, typtab.data[to_check].args.front() } });
      std::size_t ctx_position = ctx.size();

      bool result = check(ctx, term, at + 2, typtab.data[to_check].args.back());

      // Delta, x:A, Theta should become just Delta
      ctx.erase(ctx.begin() + ctx_position - 1, ctx.end());

      return result;
    } break;
  // T-ForallI   /    T-Sub
  default:                    {
      if(typtab.kinds[to_check] == type_kind::Pi)
      { // T-ForallI
        ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref,
                                                       typtab.data[to_check].args.front() } });
        std::size_t ctx_position = ctx.size();

        bool result = check(ctx, term, at, typtab.data[to_check].args.back());

        // Delta, alpha, Theta should become just Delta
        ctx.erase(ctx.begin() + ctx_position - 1, ctx.end());

        return result;
      }
      else
      { // T-Sub
        auto A = synthesize(ctx, term, at);

        if(A == static_cast<std::uint_fast32_t>(-1))
          return static_cast<std::uint_fast32_t>(-1);

        // e : B if [Theta]A <: [Theta]B
        return is_subtype(ctx, subst(ctx, A), subst(ctx, to_check));
      }
    } break;
  }
  assert(false && "Unhandled type in checking.");
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
  // T-App=>
  case IRNodeKind::app:        {
      auto A = synthesize(ctx, term, at + 1);
      if(A == static_cast<std::uint_fast32_t>(-1))
        return static_cast<std::uint_fast32_t>(-1);
      auto C = eta_synthesis(ctx, term, at + 2, subst(ctx, A));

      return C;
    } break;
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
      return type_tags::pi.make_node(typtab, TypeData { "", { alpha, beta } });
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

  // TODO: Make a diagnostic instead of an hard error!
  assert(false && "Unhandled type in synthesize.");
  return static_cast<std::uint_fast32_t>(-1);
}

bool hx_ir_type_checking::is_instantiation_L(typing_context& ctx, std::uint_fast32_t ealpha, std::uint_fast32_t A)
{
  switch(typtab.kinds[A])
  {
  case type_kind::Pi: {
      auto it = std::find_if(ctx.begin(), ctx.end(),
                      [ealpha](auto& elem) {
                        if(!std::holds_alternative<id_or_type_ref>(elem.data))
                          return false;
                      return std::get<id_or_type_ref>(elem.data).type == ealpha; });
      if(it == ctx.end())
        return static_cast<std::uint_fast32_t>(-1); // TODO: emit diagnostic
      
      // alpha1
      std::uint_fast32_t alpha1 = typtab.kinds.size();
      typtab.kinds.emplace_back(type_kind::TypeCheckExistential);
      typtab.data.emplace_back(TypeData { symbol("alpha_" + std::to_string(alpha1)) });

      // alpha2
      std::uint_fast32_t alpha2 = typtab.kinds.size();
      typtab.kinds.emplace_back(type_kind::TypeCheckExistential);
      typtab.data.emplace_back(TypeData { symbol("alpha_" + std::to_string(alpha2)) });

      ctx.emplace(it, CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha2 } });
      ctx.emplace(it, CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha1 } });

      auto id = std::get<id_or_type_ref>(it->data);
      typtab.data[id.type].args.emplace_back(type_tags::pi.make_node(typtab, TypeData { "", { alpha1, alpha2 } }));

      bool first  = is_instantiation_R(ctx, typtab.data[A].args.front(), alpha1);
      bool second = is_instantiation_L(ctx, alpha2, subst(ctx, typtab.data[A].args.back()));
      
      return first && second;
    } break;

  // InstLSolve
  default: {
      auto it = std::find_if(ctx.begin(), ctx.end(),
                      [ealpha](auto& elem) {
                        if(!std::holds_alternative<id_or_type_ref>(elem.data))
                          return false;
                      return std::get<id_or_type_ref>(elem.data).type == ealpha; });
      if(it == ctx.end())
        return false; // <- TODO: emit diagnostic

      // InstLReach ?
      if(typtab.kinds[A] == type_kind::TypeCheckExistential)
      {
        auto bit = std::find_if(it, ctx.end(),
                      [A](auto& elem) {
                        if(!std::holds_alternative<id_or_type_ref>(elem.data))
                          return false;
                      return std::get<id_or_type_ref>(elem.data).type == A; });
        if(bit == ctx.end())
          return static_cast<std::uint_fast32_t>(-1); // <- TODO: emit diagnostic

        // solve the existential
        auto id = std::get<id_or_type_ref>(bit->data);
        typtab.data[id.type].args.push_back(A);

        return true;
      }

      if(!is_well_formed(ctx, CTXElement { id_or_type_ref { id_or_type_ref::no_ref, A } }))
        return false; // <- TODO: emit diagnostic?
      // solve the existential
      auto id = std::get<id_or_type_ref>(it->data);
      typtab.data[id.type].args.push_back(A);

      return true;
    } break;
  }
}

bool hx_ir_type_checking::is_instantiation_R(typing_context& ctx, std::uint_fast32_t A, std::uint_fast32_t ealpha)
{
  switch(typtab.kinds[A])
  {
  case type_kind::Pi: {
      auto it = std::find_if(ctx.begin(), ctx.end(),
                      [ealpha](auto& elem) {
                        if(!std::holds_alternative<id_or_type_ref>(elem.data))
                          return false;
                      return std::get<id_or_type_ref>(elem.data).type == ealpha; });
      if(it == ctx.end())
        return static_cast<std::uint_fast32_t>(-1); // TODO: emit diagnostic
      
      // alpha1
      std::uint_fast32_t alpha1 = typtab.kinds.size();
      typtab.kinds.emplace_back(type_kind::TypeCheckExistential);
      typtab.data.emplace_back(TypeData { symbol("alpha_" + std::to_string(alpha1)) });

      // alpha2
      std::uint_fast32_t alpha2 = typtab.kinds.size();
      typtab.kinds.emplace_back(type_kind::TypeCheckExistential);
      typtab.data.emplace_back(TypeData { symbol("alpha_" + std::to_string(alpha2)) });

      ctx.emplace(it, CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha2 } });
      ctx.emplace(it, CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha1 } });

      auto id = std::get<id_or_type_ref>(it->data);
      typtab.data[id.type].args.emplace_back(type_tags::pi.make_node(typtab, TypeData { "", { alpha1, alpha2 } }));

      bool first  = is_instantiation_L(ctx, alpha1, typtab.data[A].args.front());
      bool second = is_instantiation_R(ctx, subst(ctx, typtab.data[A].args.back()), alpha2);
      
      return first && second;
    } break;

  // InstRSolve
  default: {
      auto it = std::find_if(ctx.begin(), ctx.end(),
                      [ealpha](auto& elem) {
                        if(!std::holds_alternative<id_or_type_ref>(elem.data))
                          return false;
                      return std::get<id_or_type_ref>(elem.data).type == ealpha; });
      if(it == ctx.end())
        return false; // <- TODO: emit diagnostic

      // InstRReach ?
      if(typtab.kinds[A] == type_kind::TypeCheckExistential)
      {
        auto bit = std::find_if(it, ctx.end(),
                      [A](auto& elem) {
                        if(!std::holds_alternative<id_or_type_ref>(elem.data))
                          return false;
                      return std::get<id_or_type_ref>(elem.data).type == A; });
        if(bit == ctx.end())
          return static_cast<std::uint_fast32_t>(-1); // <- TODO: emit diagnostic

        // solve the existential
        auto id = std::get<id_or_type_ref>(bit->data);
        typtab.data[id.type].args.push_back(A);

        return true;
      }

      if(!is_well_formed(ctx, CTXElement { id_or_type_ref { id_or_type_ref::no_ref, A } }))
        return false; // <- TODO: emit diagnostic?
      // solve the existential
      auto id = std::get<id_or_type_ref>(it->data);
      typtab.data[id.type].args.push_back(A);

      return true;
    } break;
  }
}

bool hx_ir_type_checking::is_subtype(typing_context& ctx, std::uint_fast32_t A, std::uint_fast32_t B)
{
  // <:ForallR
  if(typtab.kinds[A] != type_kind::Pi && typtab.kinds[B] == type_kind::Pi)
  {
    ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, typtab.data[B].args.front() } });
    std::size_t ctx_position = ctx.size();

    bool result = is_subtype(ctx, A, typtab.data[B].args.back());

    ctx.erase(ctx.begin() + ctx_position - 1, ctx.end());
    return result;
  }

  switch(typtab.kinds[A])
  {
  case type_kind::Kind:
  case type_kind::Type:
  case type_kind::Prop: return A == B;

  case type_kind::TypeCheckExistential:
  case type_kind::TypeConstructor:
  case type_kind::Id: {
      if(typtab.kinds[A] != type_kind::TypeCheckExistential && typtab.kinds[B] == type_kind::TypeCheckExistential)
      {
        // instatiateR
        if(auto it = std::find_if(ctx.begin(), ctx.end(),
                      [B](auto& elem) {
                        if(!std::holds_alternative<id_or_type_ref>(elem.data))
                          return false;
                      return std::get<id_or_type_ref>(elem.data).type == B; });
            it == ctx.end())
          return static_cast<std::uint_fast32_t>(-1); // <- TODO: add diagnostic

        if(is_free_variable(typtab, A, B))
          return static_cast<std::uint_fast32_t>(-1);
        return is_instantiation_R(ctx, A, B); // <- TODO: add diagnostic if not an instantiation
      }
      else if(typtab.kinds[A] == type_kind::TypeCheckExistential && typtab.kinds[B] != type_kind::TypeCheckExistential)
      {
        // instatiateL
        if(auto it = std::find_if(ctx.begin(), ctx.end(),
                      [A](auto& elem) {
                        if(!std::holds_alternative<id_or_type_ref>(elem.data))
                          return false;
                      return std::get<id_or_type_ref>(elem.data).type == A; });
            it == ctx.end())
          return static_cast<std::uint_fast32_t>(-1); // <- TODO: add diagnostic
        
        if(is_free_variable(typtab, B, A))
          return static_cast<std::uint_fast32_t>(-1);
        return is_instantiation_L(ctx, A, B); // <- TODO: add diagnostic if not an instantiation
      }
      return A == B;  // <- TODO: This needs a more clever way of saying "those are equal"
                      //          won't work as it is currently written, since we might create
                      //          identical copies of a type with simply a different index
    } break;
  case type_kind::Pi: {
      if(typtab.kinds[B] != type_kind::Pi)
      { // <:ForallL
        ctx.emplace_back(CTXElement { marker { typtab.kinds.size() } });
        std::size_t ctx_position = ctx.size();

        std::uint_fast32_t alpha = typtab.kinds.size();
        typtab.kinds.emplace_back(type_kind::TypeCheckExistential);
        typtab.data.emplace_back(TypeData { symbol("alpha_" + std::to_string(alpha)) });
        ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha } });

        bool result = is_subtype(ctx, typtab.subst(typtab.data[A].args.back(),
                                                   typtab.data[A].args.front(), alpha),
                                      B);
        ctx.erase(ctx.begin() + ctx_position - 1, ctx.end());
        return result;
      }
      else
      { // <:->
        auto A1 = typtab.data[A].args.front();
        auto A2 = typtab.data[A].args.back();

        auto B1 = typtab.data[B].args.front();
        auto B2 = typtab.data[B].args.back();

        return is_subtype(ctx, B1, A1) && is_subtype(ctx, subst(ctx, A2), subst(ctx, B2));
      }
    } break;
  case type_kind::Application: {
      // TODO: correctness? :trollface:

      if(typtab.kinds[B] != type_kind::Application)
        return static_cast<std::uint_fast32_t>(-1); // <- TODO: emit diagnostic

      bool a_is_subtype = is_subtype(ctx, typtab.data[A].args.front(), typtab.data[B].args.front());
      bool b_is_subtype = is_subtype(ctx, typtab.data[A].args.back(),  typtab.data[B].args.back());

      return a_is_subtype && b_is_subtype; // <- TODO: emit diagnostic
    } break;
  }
  assert(false && "Unhandled type in subtyping.");
  return false;
}


std::uint_fast32_t hx_ir_type_checking::eta_synthesis(typing_context& ctx, hx_per_statement_ir& term, std::size_t at, std::uint_fast32_t type)
{
  switch(typtab.kinds[type])
  {
  case type_kind::TypeCheckExistential:
    {
      auto ctx_it = std::find_if(ctx.begin(), ctx.end(),
        [ex = type](auto& elem)
        {
          if(!std::holds_alternative<id_or_type_ref>(elem.data))
            return false;
          auto& e = std::get<id_or_type_ref>(elem.data);
          return e.references() && e.type == ex;
        });
      assert(ctx_it != ctx.end() && "Inconsistent state!");


      // alpha_2
      std::uint_fast32_t alpha2 = typtab.kinds.size();
      typtab.kinds.emplace_back(type_kind::TypeCheckExistential);
      typtab.data.emplace_back(TypeData { symbol("alpha_" + std::to_string(alpha2)) });

      ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha2 } });
      // alpha_1
      std::uint_fast32_t alpha1 = typtab.kinds.size();
      typtab.kinds.emplace_back(type_kind::TypeCheckExistential);
      typtab.data.emplace_back(TypeData { symbol("alpha_" + std::to_string(alpha1)) });

      ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha1 } });
      // alpha
      std::uint_fast32_t alpha = typtab.kinds.size();
      typtab.kinds.emplace_back(type_kind::TypeCheckExistential);
      typtab.data.emplace_back(TypeData { symbol("alpha_" + std::to_string(alpha)),
                                          { type_tags::pi.make_node(typtab,
                                              TypeData { "", { alpha1, alpha2 } })
                                              }
                                        });
      ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha } });

      if(check(ctx, term, at, alpha1))
        return alpha2;

      // TODO: emit diagnostic, does not typecheck
      return static_cast<std::uint_fast32_t>(-1);
    } break;

  case type_kind::Pi:
    {
      // alpha
      std::uint_fast32_t alpha = typtab.kinds.size();
      typtab.kinds.emplace_back(type_kind::TypeCheckExistential);
      typtab.data.emplace_back(TypeData { symbol("alpha_" + std::to_string(alpha)) });

      ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha } });
      // [alpha / A]C
      std::uint_fast32_t substituted = typtab.subst(typtab.data[type].args.back(),
                                                    typtab.data[type].args.front(), alpha);
      return eta_synthesis(ctx, term, at, substituted);
    } break;

  default: ;
  }

  assert(false && "Unhandled type in eta_synthesis.");
  return static_cast<std::uint_fast32_t>(-1);
}

std::uint_fast32_t hx_ir_type_checking::subst(typing_context& ctx, std::uint_fast32_t type)
{
  if(type <= type_table::Unit_idx)
    return type;
  switch(typtab.kinds[type])
  {
  case type_kind::TypeCheckExistential:
    {
      auto& dat = typtab.data[type].args;

      auto ctx_it = std::find_if(ctx.begin(), ctx.end(),
        [ex = type](auto& elem)
        {
          if(!std::holds_alternative<id_or_type_ref>(elem.data))
            return false;
          auto& e = std::get<id_or_type_ref>(elem.data);
          return e.references() && e.type == ex;
        });
      if(dat.empty())
      {
        // TODO: emit error if ctx_it == ctx.end()
        return ctx_it != ctx.end();
      }
      if(ctx_it != ctx.end())
        return dat.front();    // <- alpha = tau

      // TODO: Emit error!
      return static_cast<std::uint_fast32_t>(-1);
    } break;

  case type_kind::TypeConstructor:
  case type_kind::Id:              return type;

  case type_kind::Pi:
  case type_kind::Application:
    {
      std::uint_fast32_t A = subst(ctx, typtab.data[type].args.front());
      std::uint_fast32_t B = subst(ctx, typtab.data[type].args.back());

      if(A == static_cast<std::uint_fast32_t>(-1) || B == static_cast<std::uint_fast32_t>(-1))
        return static_cast<std::uint_fast32_t>(-1);

      typtab.data[type].args.front() = A;
      typtab.data[type].args.back()  = B;

      return type;
    } break;
  }

  assert(false && "Unhandled case in subst.");
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

