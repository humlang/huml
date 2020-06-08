#pragma once

#include <symbol.hpp>
#include <ast.hpp>

struct exist;
struct CTXElement
{
  CTXElement(std::shared_ptr<exist> ex) : existential(ex)
  {  }

  CTXElement(identifier::ptr id, ast_ptr type)
    : id_def(id), type(type)
  {  }

  std::shared_ptr<exist> existential { nullptr }; 
  identifier::ptr id_def             { nullptr }; // <- absolute position of the binding occurence of id
  ast_ptr type                       { nullptr }; // <- absolute position of a type
};


struct typing_context
{
  using pos = std::vector<CTXElement>::const_iterator;

  ast_ptr subst(ast_ptr what);

  pos lookup_id(identifier::ptr id) const;
  pos lookup_type(ast_ptr type) const;
  pos lookup_ex(ast_ptr ex) const;

  pos lookup_id(pos begin, identifier::ptr id) const;
  pos lookup_type(pos begin, ast_ptr type) const;
  pos lookup_ex(pos begin, ast_ptr ex) const;

  std::vector<CTXElement> data;
};

struct hx_ast_type_checking
{
  hx_ast_type_checking(const hx_ast& nodes)
    : ast(nodes)
  {  }

  ast_ptr find_type(typing_context& ctx, ast_ptr of);

private:
  bool check(typing_context& ctx, ast_ptr what, ast_ptr type);
  ast_ptr synthesize(typing_context& ctx, ast_ptr what);
  ast_ptr eta_synthesize(typing_context& ctx, ast_ptr A, ast_ptr e);

  bool is_subtype(typing_context& ctx, ast_ptr A, ast_ptr B);

  bool inst_l(typing_context& ctx, std::shared_ptr<exist> alpha, ast_ptr A);
  bool inst_r(typing_context& ctx, ast_ptr A, std::shared_ptr<exist> alpha);

  bool checking_pattern { false };

  const hx_ast& ast;
};

