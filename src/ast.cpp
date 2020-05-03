#include <ast.hpp>
#include <scope.hpp>

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

app_::app_(app_::tag, std::size_t f, std::size_t arg)
  : base({}, {}), f(f), arg(arg)
{  }

expr_stmt_::expr_stmt_(expr_stmt_::tag, std::size_t exp)
  : base({}, {}), expr(exp)
{  }

access_::access_(access_::tag, token tok, std::size_t tup, std::size_t at)
  : base({}, tok), tup(tup), at(at)
{  }

tuple_::tuple_(tuple_::tag, token tok, std::vector<std::size_t> args)
  : base({}, tok), exprs(args)
{  }

lambda_::lambda_(lambda_::tag, token tok, std::size_t arg, std::size_t body)
  : base({}, tok), arg(arg), body(body)
{  }

pattern_::pattern_(pattern_::tag, token tok, std::size_t patt)
  : base({}, tok), patt(patt)
{  }

match_::match_(match_::tag, token tok, std::size_t patt, std::size_t expr)
  : base({}, tok), pat(patt), expr(expr)
{  }

pattern_matcher_::pattern_matcher_(pattern_matcher_::tag, token tok, std::size_t to_match, std::vector<std::size_t> v)
  : base({}, tok), to_match(to_match), patterns(v)
{  }

identifier_::identifier_(identifier_::tag, token tok)
  : base({}, tok)
{  }

error_::error_(error_::tag, token tok)
  : base({}, tok)
{  }

block_::block_(block_::tag, token tok, std::vector<std::size_t> v)
  : base({}, tok), exprs(v)
{  }

assign_::assign_(assign_::tag, std::size_t variable, token op, std::size_t right)
  : base({}, op), variable(variable), right(right)
{  }

assign_type_::assign_type_(assign_type_::tag, token op, std::size_t variable, std::vector<std::size_t> constructors)
  : base({}, op), variable(variable), ctors(constructors)
{  }

binary_exp_::binary_exp_(binary_exp_::tag, std::size_t left, token op, std::size_t right)
  : base({}, op), left(left), right(right)
{  }

