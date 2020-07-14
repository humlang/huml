#include <type_checking.hpp>
#include "exist.hpp" //<- stored under src/

#include <diagnostic_db.hpp>
#include <diagnostic.hpp>

#include <algorithm>
#include <sstream>

#include <iostream>

thread_local std::size_t exist_counter = 0;
thread_local std::size_t marker_counter = 0;

// ast equality
bool eqb(ast_ptr A, ast_ptr B)
{
  if(A == nullptr && B == nullptr)
    return true;
  else if(A == nullptr || B == nullptr)
    return false;
  if(A->kind != B->kind)
    return false;

  switch(A->kind)
  {
  case ASTNodeKind::Kind:
  case ASTNodeKind::Prop:
  case ASTNodeKind::Type:
  case ASTNodeKind::unit:
    return true;

  case ASTNodeKind::ptr:
    return eqb(std::static_pointer_cast<pointer>(A)->of, std::static_pointer_cast<pointer>(B)->of);

  case ASTNodeKind::identifier: {
      // pointer to an identifier are equal if they have the same binding
      return A == B;
    } break;

  case ASTNodeKind::trait: {
      auto a = std::static_pointer_cast<trait>(A);
      auto b = std::static_pointer_cast<trait>(B);

      if(a->params.size() != b->params.size() || a->methods.size() != b->methods.size())
        return false;
      if(!eqb(a->name, b->name))
        return false;
      for(std::size_t i = 0, e = a->params.size(); i < e; ++i)
        if(!eqb(a->params[i], b->params[i]))
          return false;
      for(std::size_t i = 0, e = a->methods.size(); i < e; ++i)
        if(!eqb(a->methods[i], b->methods[i]))
          return false;
      return true;
    } break;

  case ASTNodeKind::implement: {
      auto a = std::static_pointer_cast<implement>(A);
      auto b = std::static_pointer_cast<implement>(B);

      if(a->methods.size() != b->methods.size())
        return false;
      if(!eqb(a->trait, b->trait))
        return false;
      for(std::size_t i = 0, e = a->methods.size(); i < e; ++i)
        if(!eqb(a->methods[i], b->methods[i]))
          return false;
      return true;
    } break;

  case ASTNodeKind::app: {
      app::ptr a = std::static_pointer_cast<app>(A);
      app::ptr b = std::static_pointer_cast<app>(B);

      return eqb(a->lhs, b->lhs) && eqb(a->rhs, b->rhs);
    } break;

  case ASTNodeKind::lambda: {
      lambda::ptr a = std::static_pointer_cast<lambda>(A);
      lambda::ptr b = std::static_pointer_cast<lambda>(B);

      return eqb(a->lhs, b->lhs) && eqb(a->rhs, b->rhs);
    } break;

  case ASTNodeKind::pattern_matcher: {
      pattern_matcher::ptr a = std::static_pointer_cast<pattern_matcher>(A);
      pattern_matcher::ptr b = std::static_pointer_cast<pattern_matcher>(B);

      if(!eqb(a->to_match, b->to_match))
        return false;
      if(a->data.size() != b->data.size())
        return false;
      for(std::size_t i = 0; i < a->data.size(); ++i)
        if(!eqb(a->data[i], b->data[i]))
          return false;
      return true;
    } break;

  case ASTNodeKind::match: {
      match::ptr a = std::static_pointer_cast<match>(A);
      match::ptr b = std::static_pointer_cast<match>(B);

      return eqb(a->pat, b->pat) && eqb(a->exp, b->exp);
    } break;


  case ASTNodeKind::expr_stmt: {
      expr_stmt::ptr a = std::static_pointer_cast<expr_stmt>(A);
      expr_stmt::ptr b = std::static_pointer_cast<expr_stmt>(B);

      return eqb(a->lhs, b->lhs);
    } break;
  case ASTNodeKind::assign: {
      assign::ptr a = std::static_pointer_cast<assign>(A);
      assign::ptr b = std::static_pointer_cast<assign>(B);

      return eqb(a->identifier, b->identifier) && eqb(a->definition, b->definition) && eqb(a->in, b->in);
    } break;
  case ASTNodeKind::assign_type: {
      assign_type::ptr a = std::static_pointer_cast<assign_type>(A);
      assign_type::ptr b = std::static_pointer_cast<assign_type>(B);

      return eqb(a->lhs, b->lhs) && eqb(a->rhs, b->rhs);
    } break;
  case ASTNodeKind::assign_data: {
      assign_data::ptr a = std::static_pointer_cast<assign_data>(A);
      assign_data::ptr b = std::static_pointer_cast<assign_data>(B);

      return eqb(a->lhs, b->lhs) && eqb(a->rhs, b->rhs);
    } break;
  case ASTNodeKind::directive: {
      directive::ptr a = std::static_pointer_cast<directive>(A);
      directive::ptr b = std::static_pointer_cast<directive>(B);

      return a->implicit_typing == b->implicit_typing;
    } break;

  case ASTNodeKind::exist: {
      exist::ptr a = std::static_pointer_cast<exist>(A);
      exist::ptr b = std::static_pointer_cast<exist>(B);

      if(a->is_solved() && b->is_solved())
        return a->symb == b->symb && eqb(a->solution, b->solution);
      else if(a->is_solved() || b->is_solved())
        return false;
      else
        return a->symb == b->symb;
    } break;
  }
  return false;
}

ast_ptr subst(ast_ptr what, ast_ptr for_, ast_ptr in)
{
  ast_ptr to_ret = in;
  if(eqb(what, in))
  {
    to_ret = for_;
  }
  else
  {
    switch(in->kind)
    {
    default: to_ret = in; break;

    case ASTNodeKind::ptr: {
        pointer::ptr p = std::static_pointer_cast<pointer>(in);

        auto x = subst(what, for_, p->of);

        to_ret = std::make_shared<pointer>(x);
      } break;

    case ASTNodeKind::app: {
        app::ptr a = std::static_pointer_cast<app>(in);

        auto x = subst(what, for_, a->lhs);
        auto y = subst(what, for_, a->rhs);

        to_ret = std::make_shared<app>(x, y);
      } break;

    case ASTNodeKind::lambda: {
        lambda::ptr a = std::static_pointer_cast<lambda>(in);

        if(a->lhs->type != nullptr)
          a->lhs->type = subst(what, for_, a->lhs->type);
        if(a->lhs->annot != nullptr)
          a->lhs->annot = subst(what, for_, a->lhs->annot);

        auto y = subst(what, for_, a->rhs);

        to_ret = std::make_shared<lambda>(a->lhs, y);
      } break;

    case ASTNodeKind::trait: assert(false && "Cannot execute trait."); break;
    case ASTNodeKind::implement: assert(false && "Cannot execute implement."); break;

    case ASTNodeKind::pattern_matcher: {
        assert(false && "TODO: Make sure the binding works as expected.");
      } break;

    case ASTNodeKind::match: {
        assert(false && "TODO: Make sure the binding works as expected.");
      } break;

    case ASTNodeKind::exist: {
        exist::ptr a = std::static_pointer_cast<exist>(in);

        if(a->is_solved())
          to_ret = subst(what, for_, a->solution);
        else
          to_ret = a;
      } break;
    }
  }
  if(in->type != nullptr)
    to_ret->type = subst(what, for_, in->type);
  if(in->annot != nullptr)
    to_ret->annot = subst(what, for_, in->annot);

  return to_ret;
}

ast_ptr clone_ast_part_graph(ast_ptr what, tsl::robin_map<ast_ptr, ast_ptr>& cloned_ids)
{
  ast_ptr ret = nullptr;
  switch(what->kind)
  {
  case ASTNodeKind::Kind: ret = std::make_shared<kind>(); break;
  case ASTNodeKind::Prop: ret = std::make_shared<prop>(); break;
  case ASTNodeKind::Type: ret = std::make_shared<type>(); break;
  case ASTNodeKind::unit: ret = std::make_shared<unit>(); break;
  case ASTNodeKind::ptr: ret = std::make_shared<pointer>(clone_ast_part_graph(std::static_pointer_cast<pointer>(what)->of,
                                                                              cloned_ids)); break;

  case ASTNodeKind::trait: {
    trait::ptr tr = std::static_pointer_cast<trait>(what);

    auto name = clone_ast_part_graph(tr->name, cloned_ids);
    cloned_ids.emplace(tr->name, name);

    std::vector<ast_ptr> params;
    for(auto& x : tr->params)
      params.emplace_back(clone_ast_part_graph(x, cloned_ids));
    std::vector<ast_ptr> methods;
    for(auto& x : tr->methods)
      methods.emplace_back(clone_ast_part_graph(x, cloned_ids));

    ret = std::make_shared<trait>(name, params, methods);
  } break;

  case ASTNodeKind::implement: {
    implement::ptr im = std::static_pointer_cast<implement>(what);

    auto name = clone_ast_part_graph(im->trait, cloned_ids);
    cloned_ids.emplace(im->trait, name);

    std::vector<ast_ptr> methods;
    for(auto& x : im->methods)
      methods.emplace_back(clone_ast_part_graph(x, cloned_ids));

    ret = std::make_shared<implement>(name, methods);
  } break;

  case ASTNodeKind::identifier: {
    identifier::ptr id = std::static_pointer_cast<identifier>(what);

    if(cloned_ids.contains(id))
      ret = cloned_ids.find(id)->second;
    else
    {
      // free variables stay free!
      ret = id;
    }
  } break;

  case ASTNodeKind::app: {
    app::ptr ap = std::static_pointer_cast<app>(what);

    auto lhs = clone_ast_part_graph(ap->lhs, cloned_ids);
    auto rhs = clone_ast_part_graph(ap->rhs, cloned_ids);

    ret = std::make_shared<app>(lhs, rhs);
  } break;

  case ASTNodeKind::lambda: {
    lambda::ptr lam = std::static_pointer_cast<lambda>(what);

    // New binding!
    auto lhs = std::make_shared<identifier>(std::static_pointer_cast<identifier>(lam->lhs)->symb);
    if(lam->lhs->annot != nullptr)
      lhs->annot = clone_ast_part_graph(lam->lhs->annot, cloned_ids);
    if(lam->lhs->type != nullptr)
      lhs->type = clone_ast_part_graph(lam->lhs->type, cloned_ids);

    cloned_ids.emplace(lam->lhs, lhs);
    auto rhs = clone_ast_part_graph(lam->rhs, cloned_ids);

    ret = std::make_shared<lambda>(lhs, rhs);
  } break;

  case ASTNodeKind::exist: {
    exist::ptr ex = std::static_pointer_cast<exist>(what);
    if(ex->is_solved())
      return clone_ast_part_graph(ex->solution, cloned_ids);
    return ex;
  } break;

  case ASTNodeKind::assign: assert(false && "unimplemented"); return nullptr;

  case ASTNodeKind::directive:
  case ASTNodeKind::assign_data:
  case ASTNodeKind::assign_type:
  case ASTNodeKind::expr_stmt: assert(false && "Cannot clone statements."); return nullptr;
  }

  if(what->type != nullptr)
    ret->type = clone_ast_part_graph(what->type, cloned_ids);
  if(what->annot != nullptr)
    ret->annot = clone_ast_part_graph(what->annot, cloned_ids);

  return ret;
}

ast_ptr clone_ast_part_graph(ast_ptr what)
{
  tsl::robin_map<ast_ptr, ast_ptr> ids;
  return clone_ast_part_graph(what, ids);
}

lambda::ptr truncate_implicit_arguments(typing_context& ctx, lambda::ptr lam)
{
  // Check if we can potentially reduce
  if(lam->rhs->kind != ASTNodeKind::lambda)
    return lam;

  bool appears_in_parameter_list = false;
  ast_ptr bdy = lam->rhs;
  while(bdy->kind == ASTNodeKind::lambda && !appears_in_parameter_list)
  {
    auto sublam = std::static_pointer_cast<lambda>(bdy);

    appears_in_parameter_list = appears_in_parameter_list || hx_ast::used(lam->lhs, sublam->lhs);

    bdy = sublam->rhs;
  }

  if(appears_in_parameter_list)
  {
    // lam->lhs is implicit, so replace it with an existential
    auto alpha = std::make_shared<exist>("α" + std::to_string(exist_counter++));

    assert(lam->lhs->kind == ASTNodeKind::identifier && "Bug in parser.");
    ctx.data.emplace_back(alpha);
    ctx.data.emplace_back(std::static_pointer_cast<identifier>(lam->lhs), alpha);

    auto lamcpy = std::make_shared<lambda>(lam->lhs, lam->rhs);
    auto rhs = subst(lamcpy->lhs, alpha, lamcpy->rhs);

    assert(rhs->kind == ASTNodeKind::lambda && "Bug in truncate_implicit_arguments.");

    return truncate_implicit_arguments(ctx, std::static_pointer_cast<lambda>(rhs));
  }
  return lam;
}

bool has_existentials(ast_ptr a)
{
  bool has_ex = (a->type == nullptr ? false : has_existentials(a->type));
  switch(a->kind)
  {
  default: return has_ex;

  case ASTNodeKind::exist: return true;

  case ASTNodeKind::ptr: {
    pointer::ptr p = std::static_pointer_cast<pointer>(a);
    return has_existentials(p->of);
  } break;
  case ASTNodeKind::app: {
    app::ptr aa = std::static_pointer_cast<app>(a);
    return has_existentials(aa->lhs) || has_existentials(aa->rhs);
  } break;
  case ASTNodeKind::lambda: {
    lambda::ptr al = std::static_pointer_cast<lambda>(a);
    return has_existentials(al->lhs) || has_existentials(al->rhs);
  } break;
  case ASTNodeKind::pattern_matcher: {
    pattern_matcher::ptr pm = std::static_pointer_cast<pattern_matcher>(a);
    if(has_existentials(pm->to_match))
      return true;
    for(auto& r : pm->data)
      if(has_existentials(r))
        return true;
    return false;
  } break;
  case ASTNodeKind::match: {
    match::ptr am = std::static_pointer_cast<match>(a);

    return has_existentials(am->pat) || has_existentials(am->exp);
  } break;

  case ASTNodeKind::expr_stmt: {
    expr_stmt::ptr al = std::static_pointer_cast<expr_stmt>(a);
    return has_existentials(al->lhs);
  } break;
  case ASTNodeKind::assign: {
    assign::ptr al = std::static_pointer_cast<assign>(a);
    return has_existentials(al->definition) || has_existentials(al->in);
  } break;
  case ASTNodeKind::assign_data: {
    assign_data::ptr al = std::static_pointer_cast<assign_data>(a);
    return has_existentials(al->lhs) || has_existentials(al->rhs);
  } break;
  case ASTNodeKind::assign_type: {
    assign_type::ptr al = std::static_pointer_cast<assign_type>(a);
    return has_existentials(al->lhs) || has_existentials(al->rhs);
  } break;
  case ASTNodeKind::directive: {
    return false;
  } break;
  }
}


ast_ptr hx_ast_type_checking::find_type(typing_context& ctx, ast_ptr of)
{
  auto A = synthesize(ctx, of); 

  if(A != nullptr && has_existentials(A))
  {
    // A is unsolved! -> We need an annotation.
    std::stringstream ss;
    hx_ast::print(ss, A);
    diagnostic <<= diagnostic_db::sema::unsolved_ex(source_range {}, ss.str());

    return nullptr;
  }
  return A;
}

void typing_context::print(std::ostream& os)
{
  os << "[";
  for(auto it = data.begin(); it != data.end(); ++it)
  {
    auto& root = *it;
    if(root.existential != nullptr)
    {
      os << root.existential->symb;
      if(root.existential->is_solved())
      {
        os << " = ";
        hx_ast::print(os, root.existential->solution);
      }
    }
    else if(root.marker != static_cast<std::size_t>(-1))
    {
      os << "<" << root.marker << ">";
    }
    else if(root.id_def != nullptr && root.type != nullptr)
    {
      hx_ast::print(os, root.id_def);
      os << " : ";
      hx_ast::print(os, root.type);
    }
    if(std::next(it) != data.end())
      os << ", ";
  }
  os << "]";
}

ast_ptr typing_context::subst(ast_ptr what)
{
  if(what == nullptr)
    return nullptr;

  if(what->type != nullptr)
    what->type = subst(what->type);
  switch(what->kind)
  {
  case ASTNodeKind::Kind: 
  case ASTNodeKind::Prop: 
  case ASTNodeKind::Type: 
  case ASTNodeKind::unit:
  case ASTNodeKind::identifier:
    return what;

  case ASTNodeKind::ptr: {
      pointer::ptr p = std::static_pointer_cast<pointer>(what);

      return std::make_shared<pointer>(subst(p->of));
    } break;

  case ASTNodeKind::app: {
      app::ptr ap = std::static_pointer_cast<app>(what);
      
      auto lhs = subst(ap->lhs);
      auto rhs = subst(ap->rhs);

      return std::make_shared<app>(lhs, rhs);
    } break;

  case ASTNodeKind::lambda: {
      lambda::ptr lam = std::static_pointer_cast<lambda>(what);

      auto lhs = subst(lam->lhs);
      auto rhs = subst(lam->rhs);

      return std::make_shared<lambda>(lhs, rhs);
    } break;

  case ASTNodeKind::pattern_matcher: {
      pattern_matcher::ptr pm = std::static_pointer_cast<pattern_matcher>(what);

      auto p = subst(pm->to_match);

      std::vector<ast_ptr> arms;
      for(auto& r : pm->data)
        arms.emplace_back(subst(r));

      return std::make_shared<pattern_matcher>(p, arms);
    } break;
  
  case ASTNodeKind::match: {
      match::ptr am = std::static_pointer_cast<match>(what);

      auto lhs = subst(am->pat);
      auto rhs = subst(am->exp);

      return std::make_shared<match>(lhs, rhs);
    } break;

  case ASTNodeKind::exist: {
      exist::ptr ex = std::static_pointer_cast<exist>(what);
      
      if(ex->is_solved())
        return subst(ex->solution);
      return ex;
    } break;

  case ASTNodeKind::assign: assert(false && "unimplemented"); return nullptr;

  case ASTNodeKind::directive:
  case ASTNodeKind::expr_stmt:
  case ASTNodeKind::assign_data:
  case ASTNodeKind::assign_type:
    assert(false && "Statements are not types.");
  }
  return nullptr;
}

typing_context::pos typing_context::lookup_id(identifier::ptr id) const
{ return lookup_id(data.begin(), id); }

typing_context::pos typing_context::lookup_type(ast_ptr type) const
{ return lookup_type(data.begin(), type); }

typing_context::pos typing_context::lookup_ex(ast_ptr ex) const
{ return lookup_ex(data.begin(), ex); }

typing_context::pos typing_context::lookup_marker(std::size_t marker_id) const
{
  return std::find_if(data.begin(), data.end(), [marker_id]
      (auto& elem) { return elem.existential == nullptr && elem.id_def == nullptr && elem.type == nullptr && elem.marker == marker_id; });
}

typing_context::pos typing_context::lookup_id(typing_context::pos begin, identifier::ptr id) const
{
  return std::find_if(begin, data.end(), [id]
      (auto& elem) { return eqb(elem.id_def, id) && elem.type != nullptr && elem.existential == nullptr; });
}

typing_context::pos typing_context::lookup_type(typing_context::pos begin, ast_ptr type) const
{
  return std::find_if(begin, data.end(), [type]
      (auto& elem) { return elem.id_def == nullptr && eqb(elem.type, type) && elem.existential == nullptr; });
}

typing_context::pos typing_context::lookup_ex(typing_context::pos begin, ast_ptr ex) const
{
  return std::find_if(begin, data.end(), [ex]
      (auto& elem) { return elem.id_def == nullptr && elem.type == nullptr && eqb(elem.existential, ex); });
}

bool hx_ast_type_checking::check(typing_context& ctx, ast_ptr what, ast_ptr type) 
{
  switch(what->kind)
  {
  // C-Match
  case ASTNodeKind::match: {
      auto mm = std::static_pointer_cast<match>(what);

      checking_pattern = true;
      if(!check(ctx, mm->pat, type))
      {
        checking_pattern = false;
        return false;
      }
      checking_pattern = false;

      if(!check(ctx, mm->exp, type))
        return false;
      return true;
    } break;
  // C-Underscore
  case ASTNodeKind::identifier: {
      auto the_id = std::static_pointer_cast<identifier>(what);

      if(the_id->symb == symbol("_"))
        return true; // If the symbol is an underscore, we refine, i.e. it doesn't matter
      if(checking_pattern) {
        auto it = ctx.lookup_id(std::static_pointer_cast<identifier>(what));

        if(it != ctx.data.end()) {
          goto c_sub;
        }
        else {
          // we check a pattern, any free variable is implicitly bound!
          ctx.data.emplace_back(CTXElement { the_id, type });

          return true;
        }
      }
      goto c_sub;
    }
  // C-Lambda
  case ASTNodeKind::lambda: {
      if(type->kind == ASTNodeKind::lambda)
      {
        lambda::ptr pi  = std::static_pointer_cast<lambda>(type);
        lambda::ptr lam = std::static_pointer_cast<lambda>(what);

        if(pi->lhs->annot != nullptr && pi->lhs->type == nullptr)
          pi->lhs->type = pi->lhs->annot;
        if(lam->lhs->annot != nullptr && lam->lhs->type == nullptr)
          lam->lhs->type = lam->lhs->annot;

        if(lam->lhs->type)
        {
          if(!is_wellformed(ctx, lam->lhs->type))
          {
            std::stringstream a;
            hx_ast::print(a, lam->lhs->type);

            // TODO: fix diagnostic location
            diagnostic <<= diagnostic_db::sema::not_wellformed(source_range { }, a.str());
            return false;
          }
          if(!is_subtype(ctx, lam->lhs->type, pi->lhs->type))
          {
            std::stringstream a, b;
            hx_ast::print(a, lam->lhs->type);
            hx_ast::print(b, pi->lhs->type);

            // TODO: fix diagnostic location
            //assert(false);
            diagnostic <<= diagnostic_db::sema::not_a_subtype(source_range { }, a.str(), b.str());

            return false;
          }
        }
        if(!is_wellformed(ctx, pi->lhs->type))
        {
          std::stringstream a;
          hx_ast::print(a, pi->lhs->type);

          diagnostic <<= diagnostic_db::sema::not_wellformed(source_range { }, a.str());

          return false;
        }
        ctx.data.emplace_back(CTXElement { std::static_pointer_cast<identifier>(lam->lhs), pi->lhs->type });

        if(!check(ctx, lam->rhs, pi->rhs))
          return false;
        lam->lhs->type = ctx.subst(pi->lhs->type);
        lam->rhs->type = ctx.subst(pi->rhs->type);
        ctx.data.erase(ctx.lookup_id(std::static_pointer_cast<identifier>(lam->lhs)), ctx.data.end());
        return true;
      }
      goto c_sub;
    } // fallthrough
  // C-Sub
  default: {
c_sub:
      auto A = synthesize(ctx, what);
      if(A == nullptr)
        return false;

      if(!is_subtype(ctx, A, type))
      {
        std::stringstream a, b;
        hx_ast::print(a, A);
        hx_ast::print(b, type);

        // TODO: fix diagnostic location
        //assert(false);
        diagnostic <<= diagnostic_db::sema::not_a_subtype(source_range { }, a.str(), b.str());

        return false;
      }
      what->type = type;
      return true;
    } break;
  }
}

ast_ptr hx_ast_type_checking::synthesize(typing_context& ctx, ast_ptr what)
{
  if(what->annot != nullptr)
  {
    // S-Annot
    if(!is_wellformed(ctx, what->annot))
    {
      std::stringstream a;
      hx_ast::print(a, what->annot);

      diagnostic <<= diagnostic_db::sema::not_wellformed(source_range { }, a.str());

      return nullptr;
    }

    auto annot = what->annot;
    what->annot = nullptr;
    if(!check(ctx, what, annot))
      return nullptr;

    return what->type = what->annot = annot;
  }

  switch(what->kind)
  {
  // S-Ident
  case ASTNodeKind::identifier: {
      auto it = ctx.lookup_id(std::static_pointer_cast<identifier>(what));
      if(it == ctx.data.end())
      {
        std::stringstream a;
        hx_ast::print(a, what);

        diagnostic <<= diagnostic_db::sema::id_not_in_context(source_range { }, a.str());
        return nullptr;
      }
      return what->type = it->type;
    } break;

  // S-Num
  case ASTNodeKind::number: {
      auto it = std::find_if(ctx.data.begin(), ctx.data.end(), [](auto& a)
                             { return a.type && a.type->kind == ASTNodeKind::identifier
                                  && std::static_pointer_cast<number>(a.type)->symb == symbol("Nat"); });
      if(it == ctx.data.end())
      {
        std::stringstream a;
        hx_ast::print(a, what);

        // TODO: be more expressive
        diagnostic <<= diagnostic_db::sema::id_not_in_context(source_range { }, a.str());
        return nullptr;
      }
      return what->type = it->type;
    } break;

  // S-App
  case ASTNodeKind::app: {
      app::ptr aa = std::static_pointer_cast<app>(what);

      auto A = synthesize(ctx, aa->lhs);
      if(A == nullptr)
        return nullptr;
      
      auto C = eta_synthesize(ctx, ctx.subst(A), aa->rhs);
      if(C == nullptr)
        return nullptr;

      return what->type = ctx.subst(C);
    } break;

  // S-Lambda
  case ASTNodeKind::lambda: {
      lambda::ptr lam = std::static_pointer_cast<lambda>(what);

      auto alpha1 = std::make_shared<exist>("α" + std::to_string(exist_counter++));
      auto alpha2 = std::make_shared<exist>("α" + std::to_string(exist_counter++));

      ctx.data.emplace_back(alpha2);
      ctx.data.emplace_back(alpha1);
      ctx.data.emplace_back(std::static_pointer_cast<identifier>(lam->lhs), alpha1);

      if(lam->lhs->annot != nullptr && lam->lhs->type == nullptr)
        lam->lhs->type = lam->lhs->annot;

      // Check for type annotations
      if(lam->lhs->type != nullptr)
      {
        if(!is_wellformed(ctx, lam->lhs->type))
        {
          std::stringstream a;
          hx_ast::print(a, lam->lhs->type);

          diagnostic <<= diagnostic_db::sema::not_wellformed(source_range { }, a.str());
          return nullptr;
        }
        if(!is_subtype(ctx, lam->lhs->type, alpha1))
        {
          std::stringstream a, b;
          hx_ast::print(a, alpha1);
          hx_ast::print(b, lam->lhs->type);

          // TODO: fix diagnostic location
          //assert(false);
          diagnostic <<= diagnostic_db::sema::not_a_subtype(source_range { }, a.str(), b.str());
          return nullptr;
        }
      }
      if(!check(ctx, lam->rhs, alpha2))
        return nullptr;
      ctx.data.erase(ctx.lookup_id(std::static_pointer_cast<identifier>(lam->lhs)), ctx.data.end());

      lam->lhs->type = ctx.subst(alpha1);
      lam->rhs->type = ctx.subst(alpha2);

      return what->type = std::make_shared<lambda>(lam->lhs, lam->rhs->type);
    } break;

  // S-Case
  case ASTNodeKind::pattern_matcher: {
      pattern_matcher::ptr pm = std::static_pointer_cast<pattern_matcher>(what);

      auto A = synthesize(ctx, pm->to_match);
      if(A == nullptr)
        return nullptr;

      for(auto& r : pm->data)
        if(!check(ctx, r, A))
          return nullptr;
      return A;
    } break;

  // S-Assign   /   S-OracleAssign
  case ASTNodeKind::assign: {
      assign::ptr as = std::static_pointer_cast<assign>(what);

      if(as->identifier->annot != nullptr)
      {
        if(!is_wellformed(ctx, as->identifier->annot))
        {
          std::stringstream a;
          hx_ast::print(a, as->identifier->annot);

          diagnostic <<= diagnostic_db::sema::not_wellformed(source_range { }, a.str());
          return nullptr;
        }
        ctx.data.emplace_back(std::static_pointer_cast<identifier>(as->identifier), as->identifier->annot);

        if(!check(ctx, as->identifier, as->identifier->annot))
          return nullptr;
        return synthesize(ctx, as->in);
      }
      auto A = synthesize(ctx, as->definition);
      if(A == nullptr)
        return nullptr;

      ctx.data.emplace_back(std::static_pointer_cast<identifier>(as->identifier), A);
      what->type = A;

      return synthesize(ctx, as->in);
    } break;
  // S-AssignData
  case ASTNodeKind::assign_data: {
      assign_data::ptr as = std::static_pointer_cast<assign_data>(what);
      
      if(!is_wellformed(ctx, as->rhs))
      {
        std::stringstream a;
        hx_ast::print(a, as->rhs);

        diagnostic <<= diagnostic_db::sema::not_wellformed(source_range { }, a.str());
        return nullptr;
      }
      ctx.data.emplace_back(std::static_pointer_cast<identifier>(as->lhs), as->rhs);
      return what->type = as->rhs;
    } break;
  // S-AssignType
  case ASTNodeKind::assign_type: {
      assign_type::ptr as = std::static_pointer_cast<assign_type>(what);

      if(!is_wellformed(ctx, as->rhs))
      {
        std::stringstream a;
        hx_ast::print(a, as->rhs);

        diagnostic <<= diagnostic_db::sema::not_wellformed(source_range { }, a.str());
        return nullptr;
      }
      ctx.data.emplace_back(std::static_pointer_cast<identifier>(as->lhs), as->rhs);
      return what->type = as->rhs;
    } break;
  // S-ExprStmt
  case ASTNodeKind::expr_stmt: {
      expr_stmt::ptr ex = std::static_pointer_cast<expr_stmt>(what);

      return what->type = synthesize(ctx, ex->lhs);
    } break;
  // S-Directive
  case ASTNodeKind::directive: {
      implicit = std::static_pointer_cast<directive>(what)->implicit_typing;
      return what; // <- directive is basically its own type
    } break;
  }
  assert(false && "Unimplemented synthesis.");
  return nullptr;
}

ast_ptr hx_ast_type_checking::eta_synthesize(typing_context& ctx, ast_ptr A, ast_ptr e)
{
  switch(A->kind)
  {
  // >=>-Exist
  case ASTNodeKind::exist: {
      exist::ptr ex = std::static_pointer_cast<exist>(A);

      auto ex_it = ctx.lookup_ex(ex);
      if(ex_it == ctx.data.end())
      {
        diagnostic <<= diagnostic_db::sema::existential_not_in_context(source_range {  }, ex->symb.get_string());
        return nullptr;
      }
      exist::ptr alpha1 = std::make_shared<exist>("α" + std::to_string(exist_counter++));
      exist::ptr alpha2 = std::make_shared<exist>("α" + std::to_string(exist_counter++));

      // update context
      ctx.data.insert(ex_it, alpha1);
      ctx.data.insert(ex_it, alpha2);

      identifier::ptr lamid = std::make_shared<identifier>("_");
      lamid->type = alpha1;
      ex->solution = std::make_shared<lambda>(lamid, alpha2);

      if(!check(ctx, e, alpha1))
        return nullptr;
      return alpha2;
    } break;
  // >=>-Lam
  case ASTNodeKind::lambda: {
      lambda::ptr lam = std::static_pointer_cast<lambda>(A);

      if(lam->lhs->annot != nullptr && lam->lhs->type == nullptr)
        lam->lhs->type = lam->lhs->annot;

      if(!is_wellformed(ctx, lam->lhs->type))
      {
        std::stringstream a;
        hx_ast::print(a, lam->lhs->type);

        diagnostic <<= diagnostic_db::sema::not_wellformed(source_range { }, a.str());
        return nullptr;
      }
      lambda::ptr lm = implicit
        ? truncate_implicit_arguments(ctx, std::static_pointer_cast<lambda>(clone_ast_part_graph(lam)))
         :
          std::static_pointer_cast<lambda>(clone_ast_part_graph(lam));

      if(!check(ctx, e, lm->lhs->type))
        return nullptr;

      // TODO: make substitution/execution more efficient.
      // TODO: reduce e to a value.....?
      return ctx.subst(subst(lm->lhs, e, lm->rhs));
    } break;

  default: {
      std::stringstream ss;
      hx_ast::print(ss, A);
      diagnostic <<= diagnostic_db::sema::not_invokable(source_range {}, ss.str());

      return nullptr;
    }
  }
}

bool hx_ast_type_checking::is_subtype(typing_context& ctx, ast_ptr A, ast_ptr B)
{
  if(B->kind == ASTNodeKind::exist)
    goto def;
  switch(A->kind)
  {
  // <:-Kind
  case ASTNodeKind::Kind: return B->kind == ASTNodeKind::Kind; // <- Is kind a type where any other type is a subtype?
  // <:-Type
  case ASTNodeKind::Type: return B->kind == ASTNodeKind::Type;
  // <:-Prop
  case ASTNodeKind::Prop: return B->kind == ASTNodeKind::Prop;
  // <:-Id
  case ASTNodeKind::identifier: return eqb(A, B);
  // <:-Ptr
  case ASTNodeKind::ptr: return B->kind == ASTNodeKind::ptr
                         && eqb(std::static_pointer_cast<pointer>(A)->of,
                                std::static_pointer_cast<pointer>(B)->of);
  // <:-App
  case ASTNodeKind::app: {
      if(B->kind != ASTNodeKind::app)
        return false;

      app::ptr aa = std::static_pointer_cast<app>(A);
      app::ptr ba = std::static_pointer_cast<app>(B);

      return is_subtype(ctx, aa->lhs, ba->lhs) && is_subtype(ctx, aa->rhs, ba->rhs);
    } break;
  // <:-Lam
  case ASTNodeKind::lambda: {
      if(B->kind != ASTNodeKind::lambda)
        return false;

      lambda::ptr al = std::static_pointer_cast<lambda>(A);
      lambda::ptr bl = std::static_pointer_cast<lambda>(B);

      if(al->lhs->annot != nullptr && al->lhs->type == nullptr)
        al->lhs->type = al->lhs->annot;
      if(bl->lhs->annot != nullptr && bl->lhs->type == nullptr)
        bl->lhs->type = bl->lhs->annot;

      return is_subtype(ctx, bl->lhs->type, al->lhs->type)
        && is_subtype(ctx, ctx.subst(al->rhs), ctx.subst(bl->rhs));
    } break;
  // <:-Exist / <:-InstL
  default: {
def:
      if(A->kind == ASTNodeKind::exist && B->kind == ASTNodeKind::exist)
      {
        // <:-Exist
        exist::ptr ae = std::static_pointer_cast<exist>(A);
        exist::ptr be = std::static_pointer_cast<exist>(B);
        if(ae->is_solved() || be->is_solved())
          return is_subtype(ctx, ctx.subst(ae), ctx.subst(be));

        if(ae->symb != be->symb)
        {
          // either use <:-InstL or <:-InstR depending on the position in the context
          auto alpha_it = ctx.lookup_ex(A);
          auto beta_it  = ctx.lookup_ex(B);

          assert(alpha_it != ctx.data.end() && "ae must appear in the context.");
          assert(beta_it != ctx.data.end() && "be must appear in the context.");

          if(alpha_it < beta_it)
            return inst_l(ctx, ae, B);
          else if(beta_it < alpha_it)
            return inst_r(ctx, A, be);

          diagnostic <<= diagnostic_db::sema::cannot_unify_existentials(source_range { },
                                                      ae->symb.get_string(), be->symb.get_string());
          return false;
        }
        return true;
      }
      else if(A->kind == ASTNodeKind::exist && B->kind != ASTNodeKind::exist)
      {
        // <:-InstL
        if(hx_ast::used(A, B))
        {
          std::stringstream a, b;
          hx_ast::print(a, A);
          hx_ast::print(b, B);

          // TODO: fix diagnostic location
          diagnostic <<= diagnostic_db::sema::free_var_in_type(source_range { }, a.str(), b.str());
          return false;
        }
        return inst_l(ctx, std::static_pointer_cast<exist>(A), B);
      }
      else if(A->kind != ASTNodeKind::exist && B->kind == ASTNodeKind::exist)
      {
        // <:-InstR
        if(hx_ast::used(B, A))
        {
          std::stringstream a, b;
          hx_ast::print(a, B);
          hx_ast::print(b, A);

          // TODO: fix diagnostic location
          diagnostic <<= diagnostic_db::sema::free_var_in_type(source_range { }, a.str(), b.str());
          return false;
        }
        return inst_r(ctx, A, std::static_pointer_cast<exist>(B));
      }
      return false; // <- TODO: emit diagnostic
    } break;
  }
}

bool hx_ast_type_checking::inst_l(typing_context& ctx, exist::ptr alpha, ast_ptr A)
{
  switch(A->kind)
  {
  // <=L-Exist
  case ASTNodeKind::exist: {
      exist::ptr beta = std::static_pointer_cast<exist>(A);

      auto alpha_it = ctx.lookup_ex(alpha);
      auto beta_it  = ctx.lookup_ex(alpha_it, beta);

      if(alpha_it == ctx.data.end())
      {
        diagnostic <<= diagnostic_db::sema::existential_not_in_context(source_range{}, alpha->symb.get_string());
        return false;
      }
      if(beta_it == ctx.data.end())
      {
        diagnostic <<= diagnostic_db::sema::existential_not_in_context(source_range{}, beta->symb.get_string());
        return false;
      }
      beta->solution = alpha;
      return true;
    } break;
  // <=L-Lambda
  case ASTNodeKind::lambda: {
      auto alpha_it = ctx.lookup_ex(alpha);
      if(alpha_it == ctx.data.end())
      {
        diagnostic <<= diagnostic_db::sema::existential_not_in_context(source_range{}, alpha->symb.get_string());
        return false;
      }
      lambda::ptr lam = std::static_pointer_cast<lambda>(A);
      if(lam->lhs->annot != nullptr && lam->lhs->type == nullptr)
        lam->lhs->type = lam->lhs->annot;
      
      auto alpha1 = std::make_shared<exist>("α" + std::to_string(exist_counter++));
      auto alpha2 = std::make_shared<exist>("α" + std::to_string(exist_counter++));

      alpha_it = ctx.data.insert(alpha_it, alpha1);
      alpha_it = ctx.data.insert(alpha_it, alpha2);


      bool fst = is_subtype(ctx, lam->lhs->type, alpha1);
      bool snd = is_subtype(ctx, alpha2, ctx.subst(lam->rhs));
      
      lam->lhs->type = ctx.subst(alpha1);
      alpha->solution = std::make_shared<lambda>(lam->lhs, ctx.subst(alpha2));
      return fst && snd;
    } break;
  // <=L-Solve
  default: {
      if(ctx.lookup_ex(alpha) == ctx.data.end())
      {
        // TODO: fix source_range
        diagnostic <<= diagnostic_db::sema::existential_not_in_context(source_range{}, alpha->symb.get_string());
        return false;
      }
      if(!is_wellformed(ctx, A))
      {
        std::stringstream a;
        hx_ast::print(a, A);

        diagnostic <<= diagnostic_db::sema::not_wellformed(source_range { }, a.str());
        return false;
      }
      alpha->solution = A;
      return true;
    } break;
  }
}

bool hx_ast_type_checking::inst_r(typing_context& ctx, ast_ptr A, exist::ptr alpha)
{
  switch(A->kind)
  {
  // <=R-Exist
  case ASTNodeKind::exist: {
      exist::ptr beta = std::static_pointer_cast<exist>(A);

      auto alpha_it = ctx.lookup_ex(alpha);
      auto beta_it  = ctx.lookup_ex(alpha_it, beta);

      if(alpha_it == ctx.data.end())
      {
        diagnostic <<= diagnostic_db::sema::existential_not_in_context(source_range{}, alpha->symb.get_string());
        return false;
      }
      if(beta_it == ctx.data.end())
      {
        diagnostic <<= diagnostic_db::sema::existential_not_in_context(source_range{}, beta->symb.get_string());
        return false;
      }
      beta->solution = alpha;
      return true;
    } break;
  // <=R-Lambda
  case ASTNodeKind::lambda: {
      auto alpha_it = ctx.lookup_ex(alpha);
      if(alpha_it == ctx.data.end())
      {
        diagnostic <<= diagnostic_db::sema::existential_not_in_context(source_range{}, alpha->symb.get_string());
        return false;
      }
      lambda::ptr lam = std::static_pointer_cast<lambda>(A);
      if(lam->lhs->annot != nullptr && lam->lhs->type == nullptr)
        lam->lhs->type = lam->lhs->annot;
      
      auto alpha1 = std::make_shared<exist>("α" + std::to_string(exist_counter++));
      auto alpha2 = std::make_shared<exist>("α" + std::to_string(exist_counter++));

      alpha_it = ctx.data.insert(alpha_it, alpha1);
      alpha_it = ctx.data.insert(alpha_it, alpha2);


      bool fst = is_subtype(ctx, lam->lhs->type, alpha1);
      bool snd = is_subtype(ctx, alpha2, ctx.subst(lam->rhs));

      lam->lhs->type = ctx.subst(alpha1);
      alpha->solution = std::make_shared<lambda>(lam->lhs, ctx.subst(alpha2));

      return fst && snd;
    } break;
  // <=R-Solve
  default: {
      if(ctx.lookup_ex(alpha) == ctx.data.end())
      {
        // TODO: fix source_range
        diagnostic <<= diagnostic_db::sema::existential_not_in_context(source_range{}, alpha->symb.get_string());
        return false;
      }
      if(!is_wellformed(ctx, A))
      {
        std::stringstream a;
        hx_ast::print(a, A);

        diagnostic <<= diagnostic_db::sema::not_wellformed(source_range { }, a.str());
        return false;
      }
      alpha->solution = A;
      return true;
    } break;
  }
}

bool hx_ast_type_checking::is_wellformed(typing_context& ctx, ast_ptr A)
{
  switch(A->kind)
  {
  case ASTNodeKind::Kind:
  case ASTNodeKind::Prop:
  case ASTNodeKind::Type:
  case ASTNodeKind::unit:
  case ASTNodeKind::number:
    return (A->annot ? is_wellformed(ctx, A->annot) : true);

  case ASTNodeKind::ptr:
    return (A->annot ? is_wellformed(ctx, A->annot) : true)
      && is_wellformed(ctx, std::static_pointer_cast<pointer>(A)->of);

  case ASTNodeKind::exist:
    return ctx.lookup_ex(A) != ctx.data.end();

  case ASTNodeKind::identifier:
    return ctx.lookup_id(std::static_pointer_cast<identifier>(A)) != ctx.data.end()
        && (A->annot ? is_wellformed(ctx, A->annot) : true);

  case ASTNodeKind::app: {
      app::ptr a = std::static_pointer_cast<app>(A);

      if(!is_wellformed(ctx, a->lhs) || !is_wellformed(ctx, a->rhs))
        return false;

      if(a->annot)
        return is_wellformed(ctx, a->annot);
      return true;
    } break;

  case ASTNodeKind::lambda: {
      lambda::ptr lam = std::static_pointer_cast<lambda>(A);

      if(lam->lhs->type == nullptr && lam->lhs->annot != nullptr)
      {
        if(!is_wellformed(ctx, lam->lhs->annot))
          return false;
        lam->lhs->type = lam->lhs->annot;
      }
      ctx.data.emplace_back(std::static_pointer_cast<identifier>(lam->lhs), lam->lhs->type); 

      bool ret = true;
      if(!is_wellformed(ctx, lam->rhs))
        ret = false;
      ctx.data.erase(ctx.lookup_id(std::static_pointer_cast<identifier>(lam->lhs)), ctx.data.end());

      return ret;
    } break;

  default: assert(false && "Unimplemented.");
  }
  return false;
}

