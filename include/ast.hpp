#pragma once

#include <source_range.hpp>
#include <ast_nodes.hpp>
#include <memory>

#include <backend/node.hpp>

struct ast_base
{
  ast_base(ASTNodeKind kind) : kind(kind) {}

  virtual ir::Node::cRef cogen(ir::builder&) = 0;
  virtual ir::Node::cRef cogen_lval(ir::builder&) = 0;

  ASTNodeKind kind;

  std::shared_ptr<ast_base> type { nullptr };
  std::shared_ptr<ast_base> annot { nullptr };
};
using ast_ptr = std::shared_ptr<ast_base>;

struct directive : ast_base
{
  using ptr = std::shared_ptr<directive>;

  directive(bool typing)
    : ast_base(ASTNodeKind::directive), implicit_typing(typing)
  {  }

  ir::Node::cRef cogen(ir::builder&) override
  { assert(false && "no cogen"); }
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }

  bool implicit_typing;
};

struct identifier : ast_base
{
  using ptr = std::shared_ptr<identifier>;

  identifier(symbol symb)
    : ast_base(ASTNodeKind::identifier), symb(symb)
  {  }

  ir::Node::cRef cogen(ir::builder&) override;
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }

  symbol symb;

  ir::Node::cRef irn;
};

struct unit : ast_base
{
  using ptr = std::shared_ptr<unit>; unit() : ast_base(ASTNodeKind::unit) {}

  ir::Node::cRef cogen(ir::builder&) override;
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }
};

struct prop : ast_base
{
  using ptr = std::shared_ptr<prop>; prop() : ast_base(ASTNodeKind::Prop) {}

  ir::Node::cRef cogen(ir::builder&) override;
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }
};

struct type : ast_base
{
  using ptr = std::shared_ptr<type>; type() : ast_base(ASTNodeKind::Type) {}

  ir::Node::cRef cogen(ir::builder&) override;
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }
};

struct kind : ast_base
{
  using ptr = std::shared_ptr<kind>; kind() : ast_base(ASTNodeKind::Kind) {}

  ir::Node::cRef cogen(ir::builder&) override;
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }
};

struct app : ast_base
{
  using ptr = std::shared_ptr<app>;

  app(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::app), lhs(lhs), rhs(rhs)
  {  }

  ir::Node::cRef cogen(ir::builder&) override;
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct lambda : ast_base
{
  using ptr = std::shared_ptr<lambda>;

  lambda(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::lambda), lhs(lhs), rhs(rhs)
  {  }

  ir::Node::cRef cogen(ir::builder&) override;
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct match : ast_base
{
  using ptr = std::shared_ptr<match>;

  match(ast_ptr pat, ast_ptr exp) : ast_base(ASTNodeKind::match), pat(pat), exp(exp)
  {  }

  ir::Node::cRef cogen(ir::builder&) override
  { assert(false && "no cogen"); }
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }

  ast_ptr pat;
  ast_ptr exp;
};

struct pattern_matcher : ast_base
{
  using ptr = std::shared_ptr<pattern_matcher>;

  pattern_matcher(ast_ptr to_match, std::vector<ast_ptr> patterns)
    : ast_base(ASTNodeKind::pattern_matcher), to_match(to_match), data(patterns)
  {  }

  ir::Node::cRef cogen(ir::builder&) override;
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }

  ast_ptr to_match;
  std::vector<ast_ptr> data;
};

struct assign : ast_base
{
  using ptr = std::shared_ptr<assign>;

  assign(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::assign), lhs(lhs), rhs(rhs)
  {  }

  ir::Node::cRef cogen(ir::builder&) override;
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct assign_type : ast_base
{
  using ptr = std::shared_ptr<assign_type>;

  assign_type(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::assign_type), lhs(lhs), rhs(rhs)
  {  }

  ir::Node::cRef cogen(ir::builder&) override
  { assert(false && "no cogen"); }
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct assign_data : ast_base
{
  using ptr = std::shared_ptr<assign_data>;

  assign_data(ast_ptr lhs, ast_ptr rhs) : ast_base(ASTNodeKind::assign_data), lhs(lhs), rhs(rhs)
  {  }

  ir::Node::cRef cogen(ir::builder&) override
  { assert(false && "no cogen"); }
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }

  ast_ptr lhs;
  ast_ptr rhs;
};

struct expr_stmt : ast_base
{
  using ptr = std::shared_ptr<expr_stmt>;

  expr_stmt(ast_ptr lhs) : ast_base(ASTNodeKind::expr_stmt), lhs(lhs)
  {  }

  ir::Node::cRef cogen(ir::builder&) override;
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }

  ast_ptr lhs;
};

struct hx_ast
{
  void print(std::ostream& os) const;
  static void print(std::ostream& os, ast_ptr node);
  static void print_as_type(std::ostream& os, ast_ptr node);

  static bool used(ast_ptr what, ast_ptr in, bool ign_type = false)
  { tsl::robin_set<identifier::ptr> binders; return used(what, in, binders, ign_type); }
  static bool used(ast_ptr what, ast_ptr in, tsl::robin_set<identifier::ptr>& binders, bool ign_type);

  bool type_checks() const;

  std::vector<ast_ptr> data;
};

