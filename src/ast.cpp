#include <ast.hpp>
#include <type_checking.hpp>

#include <iterator>

#include <iostream>

void hx_ast::print(std::ostream& os, ast_ptr node)
{
  if(node == nullptr)
  {
    os << "NULL";
    return;
  }
  if(node->type != nullptr)
    os << "((";
  switch(node->kind)
  {
  case ASTNodeKind::exist: os << "EXIST"; break;
  case ASTNodeKind::Kind: os << "Kind"; break;
  case ASTNodeKind::Type: os << "Type"; break;
  case ASTNodeKind::Prop: os << "Prop"; break;
  case ASTNodeKind::unit: os << "()"; break;
  case ASTNodeKind::assign: {
      assign::ptr as = std::static_pointer_cast<assign>(node);
      print(os, as->lhs);
      os << " = ";
      print(os, as->rhs);
      os << ";";
    } break;
  case ASTNodeKind::assign_data: {
      assign_data::ptr as = std::static_pointer_cast<assign_data>(node);
      os << "data ";
      print(os, as->lhs);
      os << " : ";
      print(os, as->rhs);
      os << ";";
    } break;
  case ASTNodeKind::assign_type: {
      assign_type::ptr as = std::static_pointer_cast<assign_type>(node);
      os << "type ";
      print(os, as->lhs);
      os << " : ";
      print(os, as->rhs);
      os << ";";
    } break;
  case ASTNodeKind::expr_stmt:   {
      expr_stmt::ptr ex = std::static_pointer_cast<expr_stmt>(node);
      print(os, ex->lhs);
      os << ";";
    } break;
  case ASTNodeKind::identifier:  {
      identifier::ptr id = std::static_pointer_cast<identifier>(node);
      os << id->symb.get_string();
    } break;
  case ASTNodeKind::lambda:      {
      lambda::ptr lam = std::static_pointer_cast<lambda>(node);

      if(!hx_ast::used(lam->lhs, lam->rhs))
      {
        print(os, lam->lhs);
        os << " -> ";
        print(os, lam->rhs);
      }
      else
      {
        os << "\\";
        print(os, lam->lhs);
        os << ". ";
        print(os, lam->rhs);
      }
    } break;
  case ASTNodeKind::app:         {
      app::ptr ap = std::static_pointer_cast<app>(node);
      os << "((";
      print(os, ap->lhs);
      os << ") (";
      print(os, ap->rhs);
      os << "))";
    } break;                                  
  }
  if(node->type != nullptr)
  {
    os << ") : (";
    print(os, node->type);
    os << "))";
  }
}

void hx_ast::print(std::ostream& os) const
{
  for(auto& root : data)
  {
    print(os, root);
    os << "\n";
  }
}

bool hx_ast::used(ast_ptr what, ast_ptr in, tsl::robin_set<identifier::ptr>& binders)
{
  bool ret = false;
  switch(in->kind)
  {
  case ASTNodeKind::Kind: ret = what->kind == ASTNodeKind::Kind; break;
  case ASTNodeKind::Type: ret = what->kind == ASTNodeKind::Type; break;
  case ASTNodeKind::Prop: ret = what->kind == ASTNodeKind::Prop; break;
  case ASTNodeKind::unit: ret = what->kind == ASTNodeKind::unit; break;
  case ASTNodeKind::assign: {
      assign::ptr as = std::static_pointer_cast<assign>(in);

      auto itp = binders.insert(std::static_pointer_cast<identifier>(as->lhs));
      assert(itp.second && "Binder already inserted before.");
      if(used(what, as->rhs, binders))
        ret = true;
    } break;
  case ASTNodeKind::assign_data: {
      assign_data::ptr as = std::static_pointer_cast<assign_data>(in);

      if(used(what, as->rhs, binders))
        ret = true;
      auto itp = binders.insert(std::static_pointer_cast<identifier>(as->lhs));
      assert(itp.second && "Binder already inserted before.");
    } break;
  case ASTNodeKind::assign_type: {
      assign_type::ptr as = std::static_pointer_cast<assign_type>(in);

      auto itp = binders.insert(std::static_pointer_cast<identifier>(as->lhs));
      assert(itp.second && "Binder already inserted before.");
      if(used(what, as->rhs, binders))
        ret = true;
    } break;
  case ASTNodeKind::expr_stmt:   {
      expr_stmt::ptr ex = std::static_pointer_cast<expr_stmt>(in);

      if(used(what, ex->lhs, binders))
        ret = true;
    } break;
  case ASTNodeKind::identifier:  {
      identifier::ptr id = std::static_pointer_cast<identifier>(in);

      if(id->binding_occurence)
      {
        assert(id->binding_occurence->kind == ASTNodeKind::identifier && "Bug in parser.");

        if(what->kind != ASTNodeKind::identifier)
          ret = false;
        identifier::ptr whatid = std::static_pointer_cast<identifier>(what);

        // Variable is only used if the context hasn't seen its binder before
        ret = whatid->symb == id->symb && !binders.contains(std::static_pointer_cast<identifier>(id->binding_occurence));
      }
      else
      {
        // id is not bound
        if(what->kind == ASTNodeKind::identifier)
        {
          identifier::ptr other = std::static_pointer_cast<identifier>(what);

          ret = id->symb == other->symb;
        }
      }
    } break;
  case ASTNodeKind::lambda:      {
      lambda::ptr lam = std::static_pointer_cast<lambda>(in);
      assert(lam->lhs->kind == ASTNodeKind::identifier && "Bug in parser.");

      auto itp = binders.insert(std::static_pointer_cast<identifier>(lam->lhs));
      assert(itp.second && "Insertion must succeed, we can't have \"\\x. \\x. x\"");
      
      if(used(what, lam->rhs, binders))
        ret = true;
      assert(!binders.empty());
      binders.erase(binders.find(std::static_pointer_cast<identifier>(lam->lhs)));

      if(lam->lhs->type)
        ret = ret || used(what, lam->lhs->type, binders);
    } break;
  case ASTNodeKind::app:         {
      app::ptr ap = std::static_pointer_cast<app>(in);

      if(used(what, ap->lhs, binders))
        ret = true;
      if(used(what, ap->rhs, binders))
        ret = true;
    } break;                                  
  }
  return ret || (in->type ? used(what, in->type, binders) : false);
}

bool hx_ast::type_checks() const
{
  hx_ast_type_checking checker(*this);
  bool type_checks = true;

  typing_context ctx;
  for(auto& root : data)
  {
    auto typ = checker.find_type(ctx, root);

    if(typ == nullptr)
      type_checks = false;

    // TODO: and now? :-)
  }

  return type_checks;
}

