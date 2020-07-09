#pragma once

#include <ast.hpp>

namespace ir
{
  struct Node;
  struct builder;
}

const ir::Node* cogen(ir::builder& b, ast_ptr root);

