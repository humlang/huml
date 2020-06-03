#pragma once

#include <source_range.hpp>
#include <ast_nodes.hpp>
#include <memory>

struct ast_base
{
  ast_base(ASTNodeKind kind) : kind(kind) {}

  ASTNodeKind kind;

  std::shared_ptr<ast_base> type { nullptr };
};
using ast_ptr = std::shared_ptr<ast_base>;

struct identifier : ast_base
{
  using ptr = std::shared_ptr<identifier>;

  identifier(symbol symb, ast_ptr binding_occurence = nullptr)
    : ast_base(ASTNodeKind::identifier), symb(symb), binding_occurence(binding_occurence)
  {  }

  symbol symb;
  ast_ptr binding_occurence;
};

struct prop : ast_base
{ using ptr = std::shared_ptr<prop>; prop() : ast_base(ASTNodeKind::Prop) {} };

struct type : ast_base
{ using ptr = std::shared_ptr<type>; type() : ast_base(ASTNodeKind::Type) {} };

struct kind : ast_base
{ using ptr = std::shared_ptr<kind>; kind() : ast_base(ASTNodeKind::Kind) {} };

struct app : ast_base
{
  using ptr = std::shared_ptr<app>;

  app(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::app), lhs(lhs), rhs(rhs)
  {  }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct lambda : ast_base
{
  using ptr = std::shared_ptr<lambda>;

  lambda(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::lambda), lhs(lhs), rhs(rhs)
  {  }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct assign : ast_base
{
  using ptr = std::shared_ptr<assign>;

  assign(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::assign), lhs(lhs), rhs(rhs)
  {  }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct assign_type : ast_base
{
  using ptr = std::shared_ptr<assign_type>;

  assign_type(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::assign_type), lhs(lhs), rhs(rhs)
  {  }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct assign_data : ast_base
{
  using ptr = std::shared_ptr<assign_data>;

  assign_data(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::assign_data), lhs(lhs), rhs(rhs)
  {  }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct expr_stmt : ast_base
{
  using ptr = std::shared_ptr<expr_stmt>;

  expr_stmt(ast_ptr lhs) : ast_base(ASTNodeKind::expr_stmt), lhs(lhs)
  {  }

  ast_ptr lhs;
};

struct hx_ast
{
  void print(std::ostream& os) const;
  static void print(std::ostream& os, ast_ptr node);

  static bool used(ast_ptr what, ast_ptr in)
  { tsl::robin_set<identifier::ptr> binders; return used(what, in, binders); }
  static bool used(ast_ptr what, ast_ptr in, tsl::robin_set<identifier::ptr>& binders);

  bool type_checks() const;

  std::vector<ast_ptr> data;
};

