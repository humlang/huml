#include <ast.hpp>

// ast
literal::literal(literal::tag, token tok)
  : base({}, tok)
{  }

identifier::identifier(identifier::tag, token tok)
  : base({}, tok)
{  }

