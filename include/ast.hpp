#pragma once

#include <source_range.hpp>
#include <ast_nodes.hpp>


struct ast_base
{
  ast_base(ASTNodeKind kind) : kind(kind) {}

  ASTNodeKind kind;
};
using ast_ptr = ast_base*;

struct identifier : ast_base
{
  identifier(symbol symb)
    : ast_base(ASTNodeKind::identifier), symb(symb)
  {  }

  symbol symb;
};

struct prop : ast_base
{ prop() : ast_base(ASTNodeKind::Prop) {} };

struct type : ast_base
{ type() : ast_base(ASTNodeKind::Type) {} };

struct kind : ast_base
{ kind() : ast_base(ASTNodeKind::Kind) {} };

struct app : ast_base
{
  app(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::app), lhs(lhs), rhs(rhs)
  {  }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct lambda : ast_base
{
  lambda(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::lambda), lhs(lhs), rhs(rhs)
  {  }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct assign : ast_base
{
  assign(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::assign), lhs(lhs), rhs(rhs)
  {  }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct assign_type : ast_base
{
  assign_type(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::assign_type), lhs(lhs), rhs(rhs)
  {  }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct assign_data : ast_base
{
  assign_data(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::assign_data), lhs(lhs), rhs(rhs)
  {  }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct expr_stmt : ast_base
{
  expr_stmt(ast_ptr lhs) : ast_base(ASTNodeKind::expr_stmt), lhs(lhs)
  {  }

  ast_ptr lhs;
};

struct hx_ast
{
  std::vector<ast_base*> ast;
};

