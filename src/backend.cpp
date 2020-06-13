#include <backend.hpp>
#include <symbol.hpp>
#include <tsl/robin_map.h>

bool is_statement(ASTNodeKind kind)
{
  switch(kind)
  {
    case ASTNodeKind::assign:
    case ASTNodeKind::assign_data:
    case ASTNodeKind::assign_type:
    case ASTNodeKind::expr_stmt:
    case ASTNodeKind::directive:
    case ASTNodeKind::map_impl:
      return true;
    default:
      return false;
  }
}

void gen_cpp_type(std::ostream& os, ast_ptr ptr, symbol_map<symbol>& map_impls)
{
  switch(ptr->kind)
  {
  case ASTNodeKind::Type: os << "typename T"; break;
  case ASTNodeKind::identifier: {
      identifier::ptr id = std::static_pointer_cast<identifier>(ptr);

      if(map_impls.contains(id->symb))
        os << map_impls.find(id->symb)->second.get_string();

      // TODO: make this a diagnostic instead of an assertion
      assert(false && "Undefined type.");
    } break;
  }
}

void gen_cpp(std::ostream& os, ast_ptr ptr, symbol_map<symbol>& map_impls)
{
  if(ptr->type != nullptr && !is_statement(ptr->kind))
    gen_cpp_type(os, ptr->type, map_impls);

  switch(ptr->kind)
  {
  case ASTNodeKind::assign_type: {
      assign_type::ptr at = std::static_pointer_cast<assign_type>(ptr);

      auto lhs = std::static_pointer_cast<identifier>(at->lhs);

      if(map_impls.contains(lhs->symb))
      {
        os << map_impls.find(lhs->symb)->second.get_string();
      }
      else if(at->rhs->kind == ASTNodeKind::Type)
      {
        os << "struct " << lhs->symb.get_string() << " { };";
      }
      // TODO: add more cases
      // TODO: make this a diagnostic
      assert(false && "Unsupported kind of constructor.");
    } break;
  }
  os << "\n";
}

void populate_map_impls(ast_ptr ptr, symbol_map<symbol>& map_impls)
{
  switch(ptr->kind)
  {
  case ASTNodeKind::map_impl: {
      map_impl::ptr mi = std::static_pointer_cast<map_impl>(ptr);

      auto from = std::static_pointer_cast<identifier>(mi->lhs)->symb;
      auto to = std::static_pointer_cast<identifier>(mi->rhs)->symb;

      map_impls.emplace(from, to);
    } break;
  }
}

void gen_cpp(hx_ast& ast, std::ostream& os)
{
  symbol_map<symbol> map_impls;
  for(auto& r : ast.data)
    populate_map_impls(r, map_impls);

  for(auto& r : ast.data)
    gen_cpp(os, r, map_impls);
}

