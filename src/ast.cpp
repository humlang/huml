#include <ast.hpp>

std::atomic_uint_fast64_t unique_base::counter = 0;

// ast
literal_::literal_(literal_::tag, token tok)
  : base({}, tok)
{  }

unit_::unit_(unit_::tag, token tok)
  : base({}, tok)
{  }

tuple_::tuple_(tuple_::tag, token tok, std::vector<maybe_expr> args)
  : base({}, tok), exprs(args)
{  }

identifier_::identifier_(identifier_::tag, token tok)
  : base({}, tok)
{  }

error_::error_(error_::tag, token tok)
  : base({}, tok)
{  }

block_::block_(block_::tag, token tok, std::vector<maybe_expr> v)
  : base({}, tok), exprs(std::move(v))
{  }

binary_exp_::binary_exp_(binary_exp_::tag, maybe_expr left, token op, maybe_expr right)
  : base({}, op), left(std::move(left)), right(std::move(right))
{  }

assign_::assign_(assign_::tag, identifier variable, token op, maybe_expr right)
  : base({}, op), variable(std::move(variable)), right(std::move(right))
{  }

