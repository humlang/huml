#include <ast.hpp>

std::atomic_uint_fast64_t unique_base::counter = 0;

// ast
literal_::literal_(literal_::tag, token tok)
  : base({}, tok)
{  }

identifier_::identifier_(identifier_::tag, token tok)
  : base({}, tok)
{  }

error_::error_(error_::tag, token tok)
  : base({}, tok)
{  }

loop_::loop_(loop_::tag, token tok, maybe_expr times, maybe_stmt body)
  : base({}, tok), times(std::move(times)), body(std::move(body))
{  }

block_::block_(block_::tag, token tok, std::vector<maybe_stmt> v)
  : base({}, tok), stmts(std::move(v))
{  }

binary_exp_::binary_exp_(binary_exp_::tag, maybe_expr left, token op, maybe_expr right)
  : base({}, op), left(std::move(left)), right(std::move(right))
{  }

assign_::assign_(assign_::tag, identifier variable, token op, maybe_expr right)
  : base({}, op), variable(std::move(variable)), right(std::move(right))
{  }

readin_::readin_(readin_::tag, token tok, maybe_expr arg)
  : base({}, tok), argument(std::move(arg))
{  }

print_::print_(print_::tag, token tok, maybe_expr arg)
  : base({}, tok), argument(std::move(arg))
{  }

