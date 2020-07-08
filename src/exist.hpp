#pragma once

#include <ast.hpp>

struct exist : ast_base
{
  using ptr = std::shared_ptr<exist>;

  exist(symbol symb) : ast_base(ASTNodeKind::exist), symb(symb)
  {  }

  ir::Node::cRef cogen(ir::builder&) override
  { assert(false && "no cogen"); }
  ir::Node::cRef cogen_lval(ir::builder&) override
  { assert(false && "no cogen"); }

  bool is_solved() const
  { return solution != nullptr; }

  ast_ptr solution { nullptr };
  symbol symb;
};

