#include <type_checking.hpp>

#include <diagnostic_db.hpp>
#include <diagnostic.hpp>

#include <algorithm>
#include <sstream>

#include <iostream>

thread_local std::size_t exist_counter = 0;

struct exist : ast_base
{
  using ptr = std::shared_ptr<exist>;

  exist(symbol symb) : ast_base(ASTNodeKind::exist), symb(symb)
  {  }

  bool is_solved() const
  { return solution != nullptr; }

  ast_ptr solution { nullptr };
  symbol symb;
};

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

  case ASTNodeKind::identifier: {
      return std::static_pointer_cast<identifier>(A)->symb == std::static_pointer_cast<identifier>(B)->symb
        && eqb(std::static_pointer_cast<identifier>(A)->binding_occurence, std::static_pointer_cast<identifier>(B)->binding_occurence);
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


  case ASTNodeKind::expr_stmt: {
      expr_stmt::ptr a = std::static_pointer_cast<expr_stmt>(A);
      expr_stmt::ptr b = std::static_pointer_cast<expr_stmt>(B);

      return eqb(a->lhs, b->lhs);
    } break;
  case ASTNodeKind::assign: {
      assign::ptr a = std::static_pointer_cast<assign>(A);
      assign::ptr b = std::static_pointer_cast<assign>(B);

      return eqb(a->lhs, b->lhs) && eqb(a->rhs, b->rhs);
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

bool has_existentials(ast_ptr a)
{
  bool has_ex = (a->type == nullptr ? false : has_existentials(a->type));
  switch(a->kind)
  {
  default: return has_ex;

  case ASTNodeKind::exist: return true;

  case ASTNodeKind::app: {
    app::ptr aa = std::static_pointer_cast<app>(a);
    return has_existentials(aa->lhs) || has_existentials(aa->rhs);
  } break;
  case ASTNodeKind::lambda: {
    lambda::ptr al = std::static_pointer_cast<lambda>(a);
    return has_existentials(al->lhs) || has_existentials(al->rhs);
  } break;

  case ASTNodeKind::expr_stmt: {
    expr_stmt::ptr al = std::static_pointer_cast<expr_stmt>(a);
    return has_existentials(al->lhs);
  } break;
  case ASTNodeKind::assign: {
    assign::ptr al = std::static_pointer_cast<assign>(a);
    return has_existentials(al->lhs) || has_existentials(al->rhs);
  } break;
  case ASTNodeKind::assign_data: {
    assign_data::ptr al = std::static_pointer_cast<assign_data>(a);
    return has_existentials(al->lhs) || has_existentials(al->rhs);
  } break;
  case ASTNodeKind::assign_type: {
    assign_type::ptr al = std::static_pointer_cast<assign_type>(a);
    return has_existentials(al->lhs) || has_existentials(al->rhs);
  } break;
  }
}


ast_ptr hx_ast_type_checking::find_type(typing_context& ctx, ast_ptr of)
{
  auto A = synthesize(ctx, of); //cleanup(ctx, synthesize(ctx, of));

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

ast_ptr typing_context::subst(ast_ptr what)
{
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

  case ASTNodeKind::exist: {
      exist::ptr ex = std::static_pointer_cast<exist>(what);
      
      if(ex->is_solved())
        return subst(ex->solution);
      return ex;
    } break;

  case ASTNodeKind::assign:
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

typing_context::pos typing_context::lookup_id(typing_context::pos begin, identifier::ptr id) const
{
  return std::find_if(begin, data.end(), [id = id->binding_occurence ? id->binding_occurence : id]
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
  // C-Lambda
  case ASTNodeKind::lambda: {
      if(type->kind == ASTNodeKind::lambda)
      {
        lambda::ptr pi  = std::static_pointer_cast<lambda>(type);
        lambda::ptr lam = std::static_pointer_cast<lambda>(what);

        // TODO: check if lhs->type is wellformed
        if(lam->lhs->type && !is_subtype(ctx, lam->lhs->type, pi->lhs->type))
        {
          std::stringstream a, b;
          hx_ast::print(a, lam->lhs->type);
          hx_ast::print(b, pi->lhs->type);

          // TODO: fix diagnostic location
          assert(false);
          diagnostic <<= diagnostic_db::sema::not_a_subtype(source_range { }, a.str(), b.str());

          return false;
        }
        ctx.data.emplace_back(CTXElement { std::static_pointer_cast<identifier>(lam->lhs), pi->lhs->type });

        if(!check(ctx, lam->rhs, pi->rhs))
          return false;
        ctx.data.erase(ctx.lookup_id(std::static_pointer_cast<identifier>(lam->lhs)), ctx.data.end());
        return true;
      }
    } // fallthrough
  // C-Sub
  default: {
      what->type = nullptr; // <- we need to prevent an infinte loop
      auto A = synthesize(ctx, what);
      if(A == nullptr)
        return false;

      if(!is_subtype(ctx, A, type))
      {
        std::stringstream a, b;
        hx_ast::print(a, A);
        hx_ast::print(b, type);

        // TODO: fix diagnostic location
        assert(false);
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
  if(what->type != nullptr)
  {
    // S-Annot
    if(!check(ctx, what, what->type))
      return nullptr;
    // TODO: check wellformedness

    return what->type;
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

      // Check for type annotations
      if(lam->lhs->type != nullptr && !is_subtype(ctx, lam->lhs->type, alpha1))
      {
        std::stringstream a, b;
        hx_ast::print(a, alpha1);
        hx_ast::print(b, lam->lhs->type);

        // TODO: fix diagnostic location
        assert(false);
        diagnostic <<= diagnostic_db::sema::not_a_subtype(source_range { }, a.str(), b.str());
        return nullptr;
      }

      if(!check(ctx, lam->rhs, alpha2))
        return nullptr;
      ctx.data.erase(ctx.lookup_id(std::static_pointer_cast<identifier>(lam->lhs)), ctx.data.end());

      auto lamid = std::make_shared<identifier>(std::static_pointer_cast<identifier>(lam->lhs)->symb);
      lamid->type = ctx.subst(alpha1);
      return what->type = std::make_shared<lambda>(lamid, ctx.subst(alpha2));
    } break;

  // S-Assign
  case ASTNodeKind::assign: {
      assign::ptr as = std::static_pointer_cast<assign>(what);

      auto A = synthesize(ctx, as->rhs);
      if(A == nullptr)
        return nullptr;

      ctx.data.emplace_back(std::static_pointer_cast<identifier>(as->lhs), A);
      return what->type = A;
    } break;
  // S-AssignData
  case ASTNodeKind::assign_data: {
      assign_data::ptr as = std::static_pointer_cast<assign_data>(what);
      // TODO: check wellformedness
      ctx.data.emplace_back(std::static_pointer_cast<identifier>(as->lhs), as->rhs);
      return what->type = as->rhs;
    } break;
  // S-AssignType
  case ASTNodeKind::assign_type: {
      assign_type::ptr as = std::static_pointer_cast<assign_type>(what);
      // TODO: check wellformedness
      ctx.data.emplace_back(std::static_pointer_cast<identifier>(as->lhs), as->rhs);
      return what->type = as->rhs;
    } break;
  // S-ExprStmt
  case ASTNodeKind::expr_stmt: {
      expr_stmt::ptr ex = std::static_pointer_cast<expr_stmt>(what);

      return what->type = synthesize(ctx, ex->lhs);
    } break;
  }
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

      if(!check(ctx, e, lam->lhs->type))
        return nullptr;
      return lam->rhs;
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
      
      auto alpha1 = std::make_shared<exist>("α" + std::to_string(exist_counter++));
      auto alpha2 = std::make_shared<exist>("α" + std::to_string(exist_counter++));

      alpha_it = ctx.data.insert(alpha_it, alpha1);
      alpha_it = ctx.data.insert(alpha_it, alpha2);

      auto lamid = std::make_shared<identifier>(std::static_pointer_cast<identifier>(lam->lhs)->symb);
      lamid->type = alpha1;
      alpha->solution = std::make_shared<lambda>(lamid, alpha2);

      bool fst = is_subtype(ctx, lam->lhs->type, alpha1);
      lamid->type = ctx.subst(alpha1);
      bool snd = is_subtype(ctx, alpha2, ctx.subst(lam->rhs));
      std::static_pointer_cast<lambda>(alpha->solution)->rhs = ctx.subst(alpha2);

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
      
      auto alpha1 = std::make_shared<exist>("α" + std::to_string(exist_counter++));
      auto alpha2 = std::make_shared<exist>("α" + std::to_string(exist_counter++));

      alpha_it = ctx.data.insert(alpha_it, alpha1);
      alpha_it = ctx.data.insert(alpha_it, alpha2);

      auto lamid = std::make_shared<identifier>(std::static_pointer_cast<identifier>(lam->lhs)->symb);
      lamid->type = alpha1;
      alpha->solution = std::make_shared<lambda>(lamid, alpha2);

      bool fst = is_subtype(ctx, lam->lhs->type, alpha1);
      lamid->type = alpha1;
      bool snd = is_subtype(ctx, alpha2, ctx.subst(lam->rhs));
      std::static_pointer_cast<lambda>(alpha->solution)->rhs = ctx.subst(alpha2);

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
      alpha->solution = A;
      return true;
    } break;
  }
}

