#include <token.hpp>
#include <cassert>

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

std::vector<unsigned char> generic_token<ass::token_kind>::to_u8_vec() const
{
  try
  {
    switch(kind)
    {
    default:
      assert(false && "Parser should have handled this.");

    case ass::token_kind::Register:
      return { static_cast<unsigned char>(std::stoi(data.get_string())) };

    case ass::token_kind::ImmediateValue:
      {
        std::int_fast16_t tmp = std::stoi(data.get_string());

        unsigned char first = (tmp & 0b11110000) >> 4;
        unsigned char second = tmp & 0b1111;

        return { second, first };
      }
    }
  }
  catch(...)
  {
    assert(false && "Parser should have handled this.");

    return {};
  }
}

namespace ass
{

std::vector<unsigned char> instruction::to_u8_vec() const
{
  std::vector<unsigned char> to_return { static_cast<unsigned char>(op) };

  for(auto& t : args)
  {
    const auto& arg = t.to_u8_vec();

    std::move(arg.begin(), arg.end(), std::back_inserter(to_return));
  }
  return to_return;
}

}

