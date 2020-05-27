#include <type_checking.hpp>
#include <types.hpp>

namespace type_tags
{
constexpr static type_tag<type_kind::TypeCheckExistential> exist = {};
}

std::uint_fast32_t hx_ir_type_checking::cleanup(std::uint_fast32_t at)
{
  switch(typtab.kinds[at])
  {
  default:                              {
      if(!typtab.data[at].args.empty())
      {
        for(auto& x : typtab.data[at].args)
          x = cleanup(x);
      }
      return at;
    } break;

  case type_kind::TypeCheckExistential: {
      if(!typtab.data[at].args.empty())
        return cleanup(typtab.data[at].args.front());

      // make it an identifier
      typtab.kinds[at] = type_kind::Id;
      typtab.data[at].has_type = typtab.Type_sort_idx; // <- default is type

      return at;
    } break;
  }
}

bool is_free_variable(type_table& tab, std::uint_fast32_t type, std::uint_fast32_t free)
{
  assert(tab.kinds[free] == type_kind::TypeCheckExistential && "Free variable check only works for existentials a.t.m.");

  if(type == free)
    return true; // free occurs in type, i.e. it IS type

  for(auto& x : tab.data[type].args)
    if(is_free_variable(tab, x, free))
      return true;
  return false;
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
      if(typtab.kinds[to_check] == type_kind::Pi)
      {
        ctx.emplace_back(CTXElement { id_or_type_ref { at + 1, typtab.data[to_check].args.front() } });

        bool result = check(ctx, term, at + 2, typtab.data[to_check].args.back());

        // Delta, x:A, Theta should become just Delta
        auto it = std::find_if(ctx.begin(), ctx.end(),
                  [it = at + 1](auto& elem) {
                    if(!std::holds_alternative<id_or_type_ref>(elem.data))
                      return false;
                  return std::get<id_or_type_ref>(elem.data).id == it; });
        // it is pointing to x:A
        assert(it != ctx.end() && "We have added this before to our context!");
        ctx.erase(it, ctx.end());

        return result;
      }
    } // fallthrough, if the type is not a Pi, we need to use T-Sub
  // T-Sub
  default:                    {
      auto A = synthesize(ctx, term, at);

      if(A == static_cast<std::uint_fast32_t>(-1))
        return false;

      // e : B if [Theta]A <: [Theta]B
      return is_subtype(ctx, subst(ctx, A), subst(ctx, to_check));
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
                  [it = term.data[at].other_ref == static_cast<std::uint_fast32_t>(-1)
                        ? at : term.data[at].other_ref](auto& elem) {
                    if(!std::holds_alternative<id_or_type_ref>(elem.data))
                      return false;
                  return std::get<id_or_type_ref>(elem.data).id == it; });
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
      std::uint_fast32_t alpha = type_tags::exist.make_node(typtab,
                                      TypeData { symbol("alpha_" + std::to_string(typtab.kinds.size())) });
      // beta
      std::uint_fast32_t beta  = type_tags::exist.make_node(typtab,
                                      TypeData { symbol("beta_" + std::to_string(typtab.kinds.size())) });

      ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha } });
      ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, beta  } });
      ctx.emplace_back(CTXElement { id_or_type_ref { at + 1, alpha } });

      if(!check(ctx, term, at + 2, beta))
        return static_cast<std::uint_fast32_t>(-1);
      
      auto it = std::find_if(ctx.begin(), ctx.end(),
                  [it = at + 1](auto& elem) {
                    if(!std::holds_alternative<id_or_type_ref>(elem.data))
                      return false;
                  return std::get<id_or_type_ref>(elem.data).id == it; });
      // it is now pointing to x : alpha
      assert(it != ctx.end() && "We have added this to our context before.");
      ctx.erase(it, ctx.end());
      // `alpha -> beta`
      return type_tags::pi.make_node(typtab, TypeData { "", { subst(ctx, alpha), subst(ctx, beta) } });
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
      auto typ = term.data[at].type_annot = cleanup(synthesize(ctx, term, at + 1));
      ctx.clear();
      return typ;
    } break;

  case IRNodeKind::assign_type:{
      assert(term.data[at].type_annot != static_cast<std::uint_fast32_t>(-1));
      return term.data[at].type_annot;
    } break;

  case IRNodeKind::assign_data:{
      assert(term.data[at].type_annot != static_cast<std::uint_fast32_t>(-1));
      return term.data[at].type_annot;
    } break;

  case IRNodeKind::assign:     {
      auto typ = cleanup(term.data[at].type_annot = synthesize(ctx, term, at + 2));
      ctx.clear();
      return typ;
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
      std::uint_fast32_t alpha1 = type_tags::exist.make_node(typtab,
                                TypeData { symbol("alpha_" + std::to_string(typtab.kinds.size())) });
      // alpha2
      std::uint_fast32_t alpha2 = type_tags::exist.make_node(typtab,
                                TypeData { symbol("alpha_" + std::to_string(typtab.kinds.size())) });

      // pushes them in reverse order, since we insert *before* it
      it = ctx.emplace(it, CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha1 } });
      it = ctx.emplace(it, CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha2 } });
      it = it + 2; // <- it now points to alpha2....

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
      std::uint_fast32_t alpha1 = type_tags::exist.make_node(typtab,
                          TypeData { symbol("alpha_" + std::to_string(typtab.kinds.size())) });
      // alpha2
      std::uint_fast32_t alpha2 = type_tags::exist.make_node(typtab,
                          TypeData { symbol("alpha_" + std::to_string(typtab.kinds.size())) });

      // pushes them in reverse order, since we insert *before* it
      it = ctx.emplace(it, CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha1 } });
      it = ctx.emplace(it, CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha2 } });
      it = it + 2; // it now points to alpha2...

      auto id = std::get<id_or_type_ref>(it->data);
      typtab.data[id.type].args.emplace_back(type_tags::pi.make_node(typtab, TypeData { "", { alpha1, alpha2 } }));

      bool first  = is_instantiation_L(ctx, alpha1, typtab.data[A].args.front());
      bool second = is_instantiation_R(ctx, subst(ctx, typtab.data[A].args.back()), alpha2);
      
      return first && second;
    } break;

  // InstRSolve
  default: {
      auto it = std::find_if(ctx.begin(), ctx.end(),
                      [A](auto& elem) {
                        if(!std::holds_alternative<id_or_type_ref>(elem.data))
                          return false;
                      return std::get<id_or_type_ref>(elem.data).type == A; });
      if(it == ctx.end())
        return false; // <- TODO: emit diagnostic

      // InstRReach ?
      if(typtab.kinds[A] == type_kind::TypeCheckExistential)
      {
        auto bit = std::find_if(it, ctx.end(),
                      [ealpha](auto& elem) {
                        if(!std::holds_alternative<id_or_type_ref>(elem.data))
                          return false;
                      return std::get<id_or_type_ref>(elem.data).type == ealpha; });
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
      typtab.data[id.type].args.push_back(ealpha);

      return true;
    } break;
  }
}

bool hx_ir_type_checking::is_subtype(typing_context& ctx, std::uint_fast32_t A, std::uint_fast32_t B)
{
  switch(typtab.kinds[A])
  {
  case type_kind::Kind:
  case type_kind::Type:
  case type_kind::Prop: return A == B;

  case type_kind::TypeCheckExistential:
  case type_kind::TypeConstructor:
  case type_kind::Pi:
  case type_kind::Id: {
      if((typtab.kinds[A] != type_kind::TypeCheckExistential && typtab.kinds[B] == type_kind::TypeCheckExistential)
      || (typtab.kinds[A] == type_kind::TypeCheckExistential && typtab.kinds[B] == type_kind::TypeCheckExistential))
      {
        // instatiateR      in case A = ealpha  and B = ebeta, we just choose this one non-deterministically
        if(auto it = std::find_if(ctx.begin(), ctx.end(),
                      [B](auto& elem) {
                        if(!std::holds_alternative<id_or_type_ref>(elem.data))
                          return false;
                      return std::get<id_or_type_ref>(elem.data).type == B; });
            it == ctx.end())
          return false; // <- TODO: add diagnostic

        if(is_free_variable(typtab, A, B))
          return false;
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
          return false; // <- TODO: add diagnostic
        
        if(is_free_variable(typtab, B, A))
          return false;
        return is_instantiation_L(ctx, A, B); // <- TODO: add diagnostic if not an instantiation
      }
      else if(typtab.kinds[B] == type_kind::Pi)
      { // <:->
        auto A1 = typtab.data[A].args.front();
        auto A2 = typtab.data[A].args.back();

        auto B1 = typtab.data[B].args.front();
        auto B2 = typtab.data[B].args.back();

        return is_subtype(ctx, B1, A1) && is_subtype(ctx, subst(ctx, A2), subst(ctx, B2));
      }
      return A == B;  
    } break;
  case type_kind::Application: {
      // TODO: correctness? :trollface:

      if(typtab.kinds[B] != type_kind::Application)
        return false; // <- TODO: emit diagnostic

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
  // existentialApp
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
      std::uint_fast32_t alpha2 = type_tags::exist.make_node(typtab,
                          TypeData { symbol("alpha_" + std::to_string(typtab.kinds.size())) });
      ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha2 } });
      // alpha_1
      std::uint_fast32_t alpha1 = type_tags::exist.make_node(typtab,
                          TypeData { symbol("alpha_" + std::to_string(typtab.kinds.size())) });
      ctx.emplace_back(CTXElement { id_or_type_ref { id_or_type_ref::no_ref, alpha1 } });
      // alpha
      std::uint_fast32_t alpha = type_tags::exist.make_node(typtab,
                          TypeData { symbol("alpha_" + std::to_string(typtab.kinds.size())),
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

  // ->App
  case type_kind::Pi:
    {
      return check(ctx, term, at, typtab.data[type].args.front());
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
        if(ctx_it != ctx.end())
          return std::get<id_or_type_ref>(ctx_it->data).type;
        return static_cast<std::uint_fast32_t>(-1);
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

