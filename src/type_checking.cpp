#include <type_checking.hpp>

namespace ASTTags
{
  static constexpr ASTTag<ASTNodeKind::exist> exist = {  };
  static constexpr ASTTag<ASTNodeKind::ref> ref = {  };
}

std::pair<bool, std::uint_fast32_t> is_free_variable(hx_ast& tab, std::uint_fast32_t type, std::uint_fast32_t free)
{
  if(type == free)
    return { true, type }; // free occurs in type, i.e. it IS type

  std::size_t iter = type;
  for(std::size_t i = 0; i < tab.data[type].argc; ++i)
  {
    auto [a, b] = is_free_variable(tab, tab.after(iter), free);
    if(a)
      return { true, type };
    iter = b;
  }
  return { false, type };
}


bool hx_ast_type_checking::check(typing_context& ctx, std::size_t at, std::uint_fast32_t to_check)
{
  switch(ast.kinds[at])
  {
  // C-Kind
  case ASTNodeKind::Kind: return hx_ast::Kind_sort_idx == to_check;
  // C-Type
  case ASTNodeKind::Type: return hx_ast::Kind_sort_idx == to_check;
  // C-Prop
  case ASTNodeKind::Prop: return hx_ast::Type_sort_idx == to_check;
  // C-Unit
  case ASTNodeKind::unit: return hx_ast::Unit_idx == to_check;
  // C-Ref
  case ASTNodeKind::ref:  return check(ctx, at + ast.data[at].back_ref, to_check);

  // C-Lam
  case ASTNodeKind::lambda: {
      if(ast.kinds[to_check] == ASTNodeKind::lambda)
      {
        auto head = ast.after(at);
        auto bdy = ast.after(head);

        auto typ_head_binder = ast.after(to_check);
        auto typ_bdy = ast.after(typ_head_binder);

        if(ast.data[typ_head_binder].type_annot == ASTData::no_type)
          return false; // <- can't typecheck if the type information is to be synthesized
        auto typ_head = typ_head_binder + ast.data[typ_head_binder].type_annot;

        if(ast.data[head].type_annot != ASTData::no_type)
          if(!is_subtype(ctx, head, to_check))
            return false;
        ctx.emplace_back(CTXElement { head, typ_head });

        if(!check(ctx, bdy, typ_bdy))
          return false;
        return true;
      }
    } break;

  case ASTNodeKind::expr_stmt:
  case ASTNodeKind::assign_type:
  case ASTNodeKind::assign_data:
  case ASTNodeKind::assign:
    assert(false && "Call synthesize first.");

  // C-Sub
  default:  {
      auto B = synthesize(ctx, at);
      if(B == static_cast<std::uint_fast32_t>(-1))
        return false;

      if(!is_subtype(ctx, B, to_check))
        return false; // <- TODO: emit diagnostic
      return true;
    } break;
  }
  return false;
}

std::uint_fast32_t hx_ast_type_checking::synthesize(typing_context& ctx, std::size_t at)
{
  switch(ast.kinds[at])
  {
  // S-Expr
  case ASTNodeKind::expr_stmt: {
      auto lhs = ast.after(at);

      if(ast.data[lhs].type_annot != ASTData::no_type)
      {
        if(!check(ctx, lhs, lhs + ast.data[lhs].type_annot))
          return static_cast<std::uint_fast32_t>(-1);
        return lhs + ast.data[lhs].type_annot;
      }
      return synthesize(ctx, lhs);
    } break;
  // S-Assign
  case ASTNodeKind::assign: {
      auto lhs = ast.after(at);
      auto rhs = ast.after(lhs);

      if(ast.data[rhs].type_annot != ASTData::no_type)
      {
        if(!check(ctx, rhs, rhs + ast.data[rhs].type_annot))
          return static_cast<std::uint_fast32_t>(-1);
      }
      else
      {
        auto A = synthesize(ctx, rhs); 
        if(A == static_cast<std::uint_fast32_t>(-1))
          return static_cast<std::uint_fast32_t>(-1);

        ast.data[rhs].type_annot = A - rhs;
      }
      ctx.emplace_back(CTXElement { lhs, rhs + ast.data[rhs].type_annot });
      return rhs + ast.data[rhs].type_annot;
    } break;
  // S-AssignType
  case ASTNodeKind::assign_type:
  // S-AssignData
  case ASTNodeKind::assign_data: {
    auto lhs = ast.after(at);
    assert(ast.data[at].type_annot != ASTData::no_type);

    ctx.emplace_back(CTXElement { lhs, at + ast.data[at].type_annot });

    return at + ast.data[at].type_annot;
  } break;

  // S-Kind
  case ASTNodeKind::Kind: return hx_ast::Kind_sort_idx;
  // S-Type
  case ASTNodeKind::Type: return hx_ast::Kind_sort_idx;
  // S-Prop
  case ASTNodeKind::Prop: return hx_ast::Prop_sort_idx;
  // S-Unit
  case ASTNodeKind::unit: return hx_ast::Unit_idx;
  // S-Ref
  case ASTNodeKind::ref:  return synthesize(ctx, at + ast.data[at].back_ref);

  // S-Id
  case ASTNodeKind::identifier: {
      auto it = std::find_if(ctx.begin(), ctx.end(), [at = static_cast<std::uint_fast32_t>(at + ast.data[at].back_ref)](auto& el)
                             { return el.id_def == at; });
      if(it == ctx.end())
        return static_cast<std::uint_fast32_t>(-1); // TODO: emit diagnostic
      return it->type;
    } break;

  // S-Lambda / S-AnnotLambda
  case ASTNodeKind::lambda:     {
      bool domain_has_type_annot = ast.data[at + 1].type_annot != ASTData::no_type;

      if(!domain_has_type_annot)
      {
        // S-Lambda
        auto fn = ASTTags::lambda.make_node(ast, ASTData { "", 2 }, ASTDebugData {});
        auto alpha1 = ASTTags::exist.make_node(ast, ASTData { "_α" + std::to_string(ast.kinds.size()), 0 },
                                                    ASTDebugData {});
        auto alpha2 = ASTTags::exist.make_node(ast, ASTData { "_α" + std::to_string(ast.kinds.size()), 0 },
                                                    ASTDebugData {});
        ctx.emplace_back(CTXElement { at + 1, alpha1 });

        if(!check(ctx, at + 2, alpha2))
          return static_cast<std::uint_fast32_t>(-1);

        // TODO: replace existentials? get rid of thingies in context?

        return fn;
      }
      else
      {
        // S-AnnotLambda
        auto lam = ASTTags::lambda.make_node(ast, ASTData { "", 2 }, ASTDebugData {});
        auto alpha = ASTTags::exist.make_node(ast, ASTData { "_α" + std::to_string(ast.kinds.size()), 0 },
                                                   ASTDebugData {});
        auto bdy = ASTTags::ref.make_node(ast, ASTData { "", 0, ASTData::no_type,
                                                 static_cast<std::int_fast32_t>(ast.kinds.size()) - static_cast<std::int_fast32_t>((at + 1) + ast.data[at + 1].type_annot) },
                                               ASTDebugData {});
        ctx.emplace_back(CTXElement { at + 1, ast.data[at + 1].type_annot });

        if(!check(ctx, at + 2, alpha))
          return static_cast<std::uint_fast32_t>(-1);

        // TODO: replace existentials? get rid of thingies in context?

        return lam;
      }
    } break;
  }
  return static_cast<std::uint_fast32_t>(-1);
}

bool hx_ast_type_checking::is_subtype(typing_context& ctx, std::uint_fast32_t A, std::uint_fast32_t B)
{
  return false;
}


// Types are not well formed if they contain free variables
bool hx_ast_type_checking::is_well_formed(typing_context& ctx, const CTXElement& ctx_type)
{
  return false;
}

