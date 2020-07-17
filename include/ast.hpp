#pragma once

#include <source_range.hpp>
#include <ast_nodes.hpp>

#include <unordered_set>
#include <memory>

#include <backend/node.hpp>

struct ast_base
{
  ast_base(ASTNodeKind kind) : kind(kind) {}

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

  bool implicit_typing;
};

struct trait : ast_base
{
  using ptr = std::shared_ptr<trait>;

  trait(ast_ptr name, const std::vector<ast_ptr>& params,
                     const std::vector<ast_ptr>& methods)
    : ast_base(ASTNodeKind::trait), name(name), params(params), methods(methods)
  {  }

  ast_ptr name;
  std::vector<ast_ptr> params;
  std::vector<ast_ptr> methods;
};

struct implement : ast_base
{
  using ptr = std::shared_ptr<implement>;

  implement(ast_ptr trait, const std::vector<ast_ptr>& methods)
    : ast_base(ASTNodeKind::implement), trait(trait), methods(methods)
  {  }

  ast_ptr trait;
  std::vector<ast_ptr> methods;
};

struct identifier : ast_base
{
  using ptr = std::shared_ptr<identifier>;

  identifier(symbol symb)
    : ast_base(ASTNodeKind::identifier), symb(symb)
  {  }

  symbol symb;
  bool is_binding { false };

  ir::Node::cRef irn;
};

struct number : ast_base
{
  using ptr = std::shared_ptr<number>;

  number(symbol symb)
    : ast_base(ASTNodeKind::number), symb(symb)
  {  }

  symbol symb;
};

struct pointer : ast_base
{
  using ptr = std::shared_ptr<pointer>;
  
  pointer(ast_ptr of) : ast_base(ASTNodeKind::ptr), of(of) {}

  ast_ptr of;
};

struct unit : ast_base
{
  using ptr = std::shared_ptr<unit>; unit() : ast_base(ASTNodeKind::unit) {}
};

struct prop : ast_base
{
  using ptr = std::shared_ptr<prop>; prop() : ast_base(ASTNodeKind::Prop) {}
};

struct type : ast_base
{
  using ptr = std::shared_ptr<type>; type() : ast_base(ASTNodeKind::Type) {}
};

struct kind : ast_base
{
  using ptr = std::shared_ptr<kind>; kind() : ast_base(ASTNodeKind::Kind) {}
};

struct trait_type : ast_base
{
  using ptr = std::shared_ptr<trait_type>; trait_type() : ast_base(ASTNodeKind::trait_type) {}
};

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

  lambda(ast_ptr lhs, ast_ptr rhs, symbol external_name) : ast_base(ASTNodeKind::lambda), lhs(lhs), rhs(rhs), name(external_name)
  {  }

  std::pair<std::vector<ast_ptr>, ast_ptr> uncurry() const;

  ast_ptr lhs;
  ast_ptr rhs;

  symbol name { "" };
};

struct match : ast_base
{
  using ptr = std::shared_ptr<match>;

  match(ast_ptr pat, ast_ptr exp) : ast_base(ASTNodeKind::match), pat(pat), exp(exp)
  {  }

  ast_ptr pat;
  ast_ptr exp;
};

struct pattern_matcher : ast_base
{
  using ptr = std::shared_ptr<pattern_matcher>;

  pattern_matcher(ast_ptr to_match, std::vector<ast_ptr> patterns)
    : ast_base(ASTNodeKind::pattern_matcher), to_match(to_match), data(patterns)
  {  }

  ast_ptr to_match;
  std::vector<ast_ptr> data;
};

struct assign : ast_base
{
  using ptr = std::shared_ptr<assign>;

  assign(ast_ptr identifier, ast_ptr definition, ast_ptr in)
    : ast_base(ASTNodeKind::assign), identifier(identifier), definition(definition), in(in)
  {  }

  ast_ptr identifier;
  ast_ptr definition;
  ast_ptr in;
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
  hx_ast();

  // Transforms trees of structure
  //   def x = +      def x  +
  //          / \   to      / \  <- uses of identifiers now
  //         x   x         def x    point to their definition
  void consider_scoping();

  void print(std::ostream& os) const;
  static void print(std::ostream& os, ast_ptr node);
  static void print_as_type(std::ostream& os, ast_ptr node);

  static bool used(ast_ptr what, ast_ptr in, bool ign_type = false)
  { tsl::robin_set<identifier::ptr> binders; return used(what, in, binders, ign_type); }
  static bool used(ast_ptr what, ast_ptr in, tsl::robin_set<identifier::ptr>& binders, bool ign_type);

  bool type_checks();
  void cogen(std::string output_file) const;

  std::vector<ast_ptr> data;
};

