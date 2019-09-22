#include "reader.hpp"
#include "ast.hpp"

#include <cassert>
#include <istream>
#include <queue>

reader::token::token(token_kind kind, symbol data, source_range range)
  : kind(kind), data(data), loc(range)
{  }

reader::reader(const char* module, std::istream& is)
  : module(module), is(is), linebuf(), col(0), row(1)
{  }

template<>
char reader::get<char>()
{
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
reader::token reader::get<reader::token>()
{
  symbol data("");
  token_kind kind = token_kind::Undef;

  char ch = get<char>();
  std::size_t beg_row = row;
  std::size_t beg_col = col - 1;

  switch(ch)
  {
  default:
    {
      // Optimistically allow any kind of identifier to allow for unicode
      std::uint_fast32_t hash = ch;
      std::string name;
      name.push_back(ch);
      while(col < linebuf.size())
      {
        ch = linebuf[col++];

        // break if we hit whitespace (or other control chars) or any other token char
        if(std::iscntrl(ch) || std::isspace(ch) || ch == '(' || ch == ')')
        {
          col--;
          break;
        }
        name.push_back(ch);
        hash ^= (hash * 31) + ch;
      }
      kind = token_kind::Atom;
      data = symbol(name);
    } break;

  case '(':
  case ')':
      kind = static_cast<token_kind>(ch);
    break;
  case EOF:
      kind = token_kind::EndOfFile;
    break;
  }
  return token(kind, data, {module, beg_col + 1, beg_row, col + 1, row});
}

std::vector<std::shared_ptr<expression>> reader::read(const char* module, std::istream& is)
{
  reader r(module, is);

  //Parse
  std::vector<std::shared_ptr<expression>> ast;
  std::queue<std::shared_ptr<list>> lists;
  token tok(token_kind::Undef, "", {});
  while(tok.kind != token_kind::EndOfFile)
  {
    tok = r.get<reader::token>();
    switch(tok.kind)
    {
    case token_kind::L_Paren:
      {
        if(lists.empty())
        {
          auto l = std::make_shared<list>(tok.loc);
          ast.push_back(l);
          lists.push(l);
        }
        else
        {
          auto l = std::make_shared<list>(tok.loc);
          lists.front()->add(l);
          lists.push(l);
        }
      } break;

    case token_kind::R_Paren:
      {
        if(lists.empty())
        {
          // parse error, what do?
          assert(false);
        }    
        else
        {
          lists.front()->change_location(lists.front()->location() + tok.loc);
          lists.pop();
        }
      } break;

    case token_kind::Atom:
      {
        if(lists.empty())
        {
          ast.push_back(std::make_shared<atom>(tok.loc, tok.data));
        }
        else
        {
          lists.front()->add(std::make_shared<atom>(tok.loc, tok.data));
        }
      } break;

    case token_kind::Undef:
      {
        // tokenize error, what do?
        assert(false);
      } break;

    case token_kind::EndOfFile:
      break;
    }
  }
  if(!lists.empty())
  {
    // parse error, what do?
    assert(false);
  }
  return ast;
}

