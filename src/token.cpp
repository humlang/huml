#include <token.hpp>

token::token(token_kind kind, symbol data, source_range range)
  : kind(kind), data(data), loc(range)
{  }

std::string_view kind_to_str(token_kind kind)
{
  switch(kind)
  {
  default:
  case token_kind::EndOfFile: return "EOF";
  case token_kind::Identifier: return "Identifier";
  case token_kind::LiteralNumber: return "LiteralNumber";
  case token_kind::Keyword: return "Keyword";
  case token_kind::Assign: return "Assign";
  case token_kind::Colon: return "Colon";
  case token_kind::Semi: return "Semicolon";
  case token_kind::LBrace: return "LBrace";
  case token_kind::RBrace: return "RBrace";
  case token_kind::Plus: return "Plus";
  case token_kind::Minus: return "Minus";
  case token_kind::Asterisk: return "Asterisk";
  case token_kind::Equal: return "Equal";
  case token_kind::Undef: return "Undefined";
  }
}

