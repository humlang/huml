#include <assembler.hpp>
#include <token.hpp>
#include <cassert>

std::string_view kind_to_str(token_kind kind)
{
  switch(kind)
  {
  default:
  case token_kind::EndOfFile: return "EOF";
  case token_kind::Arrow: return "->";
  case token_kind::Identifier: return "Identifier";
  case token_kind::LiteralNumber: return "LiteralNumber";
  case token_kind::Keyword: return "Keyword";
  case token_kind::Doublearrow: return "Doublearrow";
  case token_kind::Backslash: return "Backslash";
  case token_kind::Semi: return "Semicolon";
  case token_kind::Colon: return "Colon";
  case token_kind::LBrace: return "LBrace";
  case token_kind::RBrace: return "RBrace";
  case token_kind::Plus: return "Plus";
  case token_kind::Minus: return "Minus";
  case token_kind::Asterisk: return "Asterisk";
  case token_kind::LParen: return "LParen";
  case token_kind::RParen: return "RParen";
  case token_kind::Comma: return "Comma";
  case token_kind::LBracket: return "LBracket";
  case token_kind::RBracket: return "RBracket";
  case token_kind::Undef: return "Undefined";
  }
}

std::vector<unsigned char> generic_token<ass::token_kind>::to_u8_vec(const ass::program& assm) const
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

        unsigned char first = (tmp & 0b11111111'00000000) >> 8;
        unsigned char second = tmp & 0b11111111;

        return { first, second };
      }

    case ass::token_kind::LabelUse:
      {
        const auto& p = assm.lookup_symbol(data);

        unsigned char first  = (p.second & 0b11111111'00000000) >> 8;
        unsigned char second = p.second & 0b11111111;

        return { first, second };
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

std::vector<unsigned char> instruction::to_u8_vec(const program& assm) const
{
  // TODO: Add directives
  if(op)
  {
    // We have an instruction
    std::vector<unsigned char> to_return { static_cast<unsigned char>(*op) };

    for(auto& t : args)
    {
      const auto& arg = t.to_u8_vec(assm);

      std::move(arg.begin(), arg.end(), std::back_inserter(to_return));
    }
    return to_return;
  }
  return {};
}

}

