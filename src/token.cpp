#include <token.hpp>

token::token(token_kind kind, symbol data, source_range range)
  : kind(kind), data(data), loc(range)
{  }

