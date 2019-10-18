#include <diagnostic.hpp>
#include <diagnostic_db.hpp>

#include <reader.hpp>
#include <token.hpp>
#include <ast.hpp>

#include <cassert>
#include <istream>
#include <queue>

reader::reader(const char* module, std::istream& is)
  : module(module), is(is), linebuf(), col(0), row(1)
{  }

template<>
char reader::get<char>()
{
  // yields the next char that is not a basic whitespace character i.e. NOT stuff like zero width space
  char ch = 0;
  bool skipped_line = false;
  do
  {
    if(col >= linebuf.size())
    {
      if(!linebuf.empty())
        row++;
      if(!(std::getline(is, linebuf)))
      {
        col = 1;
        linebuf = "";
        return EOF;
      }
      else
      {
        while(linebuf.empty())
        {
          row++;
          if(!(std::getline(is, linebuf)))
          {
            col = 1;
            linebuf = "";
            return EOF;
          }
        }
      }
      col = 0;
    }
    ch = linebuf[col++];
    if(std::isspace(ch))
    {
      skipped_line = true;
      while(col < linebuf.size())
      {
        ch = linebuf[col++];
        if(!(std::isspace(ch)))
        {
          skipped_line = false;
          break;
        }
      }
    }
  } while(skipped_line);

  return ch;
}


template<>
token reader::get<token>()
{
restart_get:
  symbol data("");
  token_kind kind = token_kind::Undef;

  char ch = get<char>();
  std::size_t beg_row = row;
  std::size_t beg_col = col - 1;

  bool starts_with_zero = false;
  switch(ch)
  {
  default:
    {
      // Optimistically allow any kind of identifier to allow for unicode
      std::string name;
      name.push_back(ch);
      while(col < linebuf.size())
      {
        // don't use get<char>() here, identifiers are not connected with a '\n'
        ch = linebuf[col++];

        // break if we hit whitespace (or other control chars) or any other token char
        if(std::iscntrl(ch) || std::isspace(ch)) // || ch == '(' || ch == ')')
        {
          col--;
          break;
        }
        name.push_back(ch);
      }
      kind = token_kind::Identifier;
      data = symbol(name);
    } break;

  case '0': starts_with_zero = true; case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    {
      // read until there is no number anymore. Disallow 000840
      std::string name;
      name.push_back(ch);
      bool emit_starts_with_zero_error = false;
      while(col < linebuf.size())
      {
        // don't use get<char>() here, numbers are not connected with a '\n'
        ch = linebuf[col++];

        // break if we hit anything besides '0'..'9'
        if(!std::isdigit(ch))
        {
          col--;
          break;
        }
        if(starts_with_zero)
          emit_starts_with_zero_error = true;
        name.push_back(ch);
      }
      if(emit_starts_with_zero_error)
      {
        diagnostic <<= (diagnostic_db::parser::leading_zeros(name) + source_range(module, beg_col + 1, beg_row, col + 1, row + 1));
        goto restart_get; // <- SO WHAT
      }
      kind = token_kind::LiteralNumber;
      data = symbol(name);
    } break;

  case EOF:
      kind = token_kind::EndOfFile;
    break;
  }
  return token(kind, data, {module, beg_col + 1, beg_row, col + 1, row + 1});
}

std::vector<ast_type> reader::read(const char* module, std::istream& is)
{
  reader r(module, is);

  std::vector<ast_type> ast;
  token tok(token_kind::Undef, "", {});
  while(tok.kind != token_kind::EndOfFile)
  {
    tok = r.get<token>();

    switch(tok.kind)
    {
    default:
    {
      diagnostic <<= (diagnostic_db::parser::unknown_token(tok.data.get_string()) + tok.loc);
    } break;

    case token_kind::EndOfFile:
    break;

    case token_kind::LiteralNumber:
    {
      ast.push_back(literal(ast_tags::literal, tok));
    } break;
    }
  }
  if(ast.empty())
  {
    diagnostic <<= (-diagnostic_db::parser::empty_module);
  }
  return ast;
}

