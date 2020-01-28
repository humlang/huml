#include <ast.hpp>

// ast
literal::literal(literal::tag, token tok)
  : base({}, tok)
{  }

identifier::identifier(identifier::tag, token tok)
  : base({}, tok)
{  }

error::error(error::tag, token tok)
  : base({}, tok)
{  }

loop::loop(loop::tag, token tok, rec_wrap_t<literal> times, stmt_type body)
  : base({}, tok), times(std::move(times)), body(std::move(body))
{  }

block::block(block::tag, token tok, std::vector<std::variant<std::monostate, stmt_type, rec_wrap_t<error>>> v)
  : base({}, tok), stmts(std::move(v))
{  }

binary_exp::binary_exp(binary_exp::tag, std::variant<std::monostate, exp_type, rec_wrap_t<error>> left,
                                        token op,
                                        std::variant<std::monostate, exp_type, rec_wrap_t<error>> right)
  : base({}, op), left(std::move(left)), right(std::move(right))
{  }

assign::assign(base::tag, rec_wrap_t<identifier> variable, token op,
                          std::variant<std::monostate, exp_type, rec_wrap_t<error>> right)
  : base({}, op), variable(std::move(variable)), right(std::move(right))
{  }

