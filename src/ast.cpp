#include <ast.hpp>

std::atomic_uint_fast64_t unique_base::counter = 0;

// ast
literal_::literal_(literal_::tag, token tok)
  : base({}, tok)
{  }

unit_::unit_(unit_::tag, token tok)
  : base({}, tok)
{  }

top_::top_(top_::tag, token tok)
  : base({}, tok)
{  }

bot_::bot_(bot_::tag, token tok)
  : base({}, tok)
{  }

app_::app_(app_::tag, maybe_expr f, maybe_expr arg)
  : base({}, {}), f(std::move(f)), arg(std::move(arg))
{  }

expr_stmt_::expr_stmt_(expr_stmt_::tag, maybe_expr exp)
  : base({}, {}), expr(std::move(exp))
{  }

access_::access_(access_::tag, token tok, maybe_expr tup, maybe_expr at)
  : base({}, tok), tup(std::move(tup)), at(std::move(at))
{  }

tuple_::tuple_(tuple_::tag, token tok, std::vector<maybe_expr> args)
  : base({}, tok), exprs(std::move(args))
{  }

lambda_::lambda_(lambda_::tag, token tok, identifier arg, maybe_expr body)
  : base({}, tok), arg(std::move(arg)), body(std::move(body))
{  }

pattern_::pattern_(pattern_::tag, token tok, maybe_patt patt)
  : base({}, tok), patt(std::move(patt))
{  }

match_::match_(match_::tag, token tok, maybe_patt patt, maybe_expr expr)
  : base({}, tok), pat(std::move(patt)), expr(std::move(expr))
{  }

pattern_matcher_::pattern_matcher_(pattern_matcher_::tag, token tok, maybe_expr to_match, std::vector<maybe_match> v)
  : base({}, tok), to_match(std::move(to_match)), patterns(std::move(v))
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

assign_::assign_(assign_::tag, maybe_expr variable, token op, maybe_expr right)
  : base({}, op), variable(std::move(variable)), right(std::move(right))
{  }

binary_exp_::binary_exp_(binary_exp_::tag, maybe_expr left, token op, maybe_expr right)
  : base({}, op), left(std::move(left)), right(std::move(right))
{  }

