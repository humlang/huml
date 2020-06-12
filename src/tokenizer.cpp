#include <reader.hpp>
#include <diagnostic.hpp>
#include <diagnostic_db.hpp>
#include <stream_lookup.hpp>
#include <fixit_info.hpp>


#include <vm.hpp>

using namespace std::literals::string_view_literals;

namespace ass
{
auto keyword_map = tsl::robin_map<std::string_view, op_code>({
  { "halt"sv,    op_code::HALT },
  { "load"sv,    op_code::LOAD },
  { "add"sv,     op_code::ADD },
  { "sub"sv,     op_code::SUB },
  { "mul"sv,     op_code::MUL },
  { "div"sv,     op_code::DIV },
  { "jmp"sv,     op_code::JMP },
  { "jrp"sv,     op_code::JMPREL },
  { "jcmp"sv,    op_code::JMP_CMP },
  { "jncmp"sv,   op_code::JMP_NCMP },
  { "eq"sv,      op_code::EQUAL },
  { "gt"sv,      op_code::GREATER },
  { "lt"sv,      op_code::LESS },
  { "ge"sv,      op_code::GREATER_EQUAL },
  { "le"sv,      op_code::LESS_EQUAL },
  { "aloc"sv,    op_code::ALLOC },
  { "inc"sv,     op_code::INC },
  { "dec"sv,     op_code::DEC },
  { "shl"sv,     op_code::SHIFT_LEFT },
  { "shr"sv,     op_code::SHIFT_RIGHT }
});
}

auto keyword_set = tsl::robin_set<std::string_view>({
  "TOP"sv,
  "BOT"sv,
  "Type"sv,
  "Kind"sv,
  "Prop"sv,
  "type"sv,
  "data"sv,
  "case"sv
});

auto operator_symbols_map = tsl::robin_map<std::string_view, token_kind>({
  {"\\"sv, token_kind::Backslash},
  {"|"sv, token_kind::Pipe},
  {":"sv, token_kind::Colon},
  {";"sv, token_kind::Semi},
  {"{"sv, token_kind::LBrace},
  {"}"sv, token_kind::RBrace},
  {"+"sv, token_kind::Plus},
  {"-"sv, token_kind::Minus},
  {"*"sv,  token_kind::Asterisk},
  {"="sv,  token_kind::Equal},
  {"."sv,  token_kind::Dot},
  {"("sv,  token_kind::LParen},
  {")"sv,  token_kind::RParen},
  {"["sv,  token_kind::LBracket},
  {"]"sv,  token_kind::RBracket},
  {","sv,  token_kind::Comma},
});

constexpr static bool isprint(unsigned char c)
{ return (' ' <= c && c <= '~'); }

base_reader::base_reader(std::string_view module)
  : module(module), is(stream_lookup[module]), linebuf(), col(0), row(1), uses_reader(true)
{  }

base_reader::base_reader(std::istream& stream)
  : module("#TXT#"), is(stream), linebuf(), col(0), row(1)
{  }

base_reader::~base_reader()
{ if(uses_reader) stream_lookup.drop(module); }

char base_reader::getc()
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


///// Tokenization

token hx_reader::gett()
{
restart_get:
  symbol data("");
  token_kind kind = token_kind::Undef;

  char ch = getc();
  std::size_t beg_row = row;
  std::size_t beg_col = col - 1;

  bool starts_with_zero = false;
  switch(ch)
  {
  default:
    if(!std::iscntrl(ch) && !std::isspace(ch))
    {
      // Optimistically allow any kind of identifier to allow for unicode
      std::string name;
      name.push_back(ch);
      while(col < linebuf.size())
      {
        // don't use get<char>() here, identifiers are not connected with a '\n'
        ch = linebuf[col++];

        // we look up in the operator map only for the single char. otherwise we wont parse y:= not correctly for example
        std::string ch_s;
        ch_s.push_back(ch);
        // break if we hit whitespace (or other control chars) or any other operator symbol char
        if(std::iscntrl(ch) || std::isspace(ch) || operator_symbols_map.contains(ch_s.c_str()) || keyword_set.contains(name.c_str()))
        {
          col--;
          break;
        }
        name.push_back(ch);
      }
      kind = (operator_symbols_map.contains(name.c_str()) ? operator_symbols_map[name.c_str()] : token_kind::Identifier);
      kind = (keyword_set.contains(name.c_str()) ? token_kind::Keyword : kind);

      data = symbol(name);
    }
    else
    {
      assert(false && "Control or space character leaked into get<token>!");
    }
    break;

  case '0': starts_with_zero = true; case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    {
      // readin until there is no number anymore. Disallow 000840
      std::string name;
      name.push_back(ch);
      bool emit_not_a_number_error = false;
      bool emit_starts_with_zero_error = false;
      while(col < linebuf.size())
      {
        // don't use get<char>() here, numbers are not connected with a '\n'
        ch = linebuf[col++];

        // we look up in the operator map only for the single char. otherwise we wont parse 2; not correctly for example
        // This could give problems with floaing point numbers since 2.3 will now be parsed as "2" "." "3" so to literals and a point
        std::string ch_s;
        ch_s.push_back(ch);
        // break if we hit whitespace (or other control chars) or any other token char
        if(std::iscntrl(ch) || std::isspace(ch) || operator_symbols_map.count(ch_s.c_str()) || keyword_set.count(name.c_str()))
        {
          col--;
          break;
        }
        if(!std::isdigit(ch))
          emit_not_a_number_error = true;
        if(starts_with_zero && !emit_not_a_number_error)
          emit_starts_with_zero_error = true;
        name.push_back(ch);
      }
      if(emit_starts_with_zero_error)
      {
        diagnostic <<= diagnostic_db::parser::leading_zeros(source_range {module, beg_col + 1, beg_row, col + 1, row + 1}, name);
        goto restart_get; // <- SO WHAT
      }
      if(emit_not_a_number_error)
      {
        diagnostic <<= diagnostic_db::parser::no_number(source_range {module, beg_col + 1, beg_row, col + 1, row + 1}, name);
        goto restart_get;
      }
      kind = token_kind::LiteralNumber;
      data = symbol(name);
    } break;
  case '=':
  {
    if(col < linebuf.size() && linebuf[col] == '>')
    {
      kind = token_kind::Doublearrow;
      data = "=>";
      ch = linebuf[col++];
    }
    else
    {
      kind = token_kind::Equal;
      data = "=";
    }
  } break;
  case '#':
  {
    kind = token_kind::Hash;
    data = "#";
  } break;
  case '|':
  {
    kind = token_kind::Pipe;
    data = "|";
  } break;
  case '*':
  {
    kind = token_kind::Asterisk;
    data = "*";
  } break;
  case '+':
  {
    kind = token_kind::Plus;
    data = "+";
  } break;
  case '-':
  {
    if(col < linebuf.size() && linebuf[col] == '>')
    {
      kind = token_kind::Arrow;
      data = "->";
      ch = linebuf[col++];
    }
    else
    {
      kind = token_kind::Minus;
      data = "-";
    }
  } break;
  case '{':
    {
      kind = token_kind::LBrace;
      data = "{";
    } break;
  case '}':
    {
      kind = token_kind::RBrace;
      data = "}";
    } break;
  case '[':
    {
      kind = token_kind::LBracket;
      data = "[";
    } break;
  case ']':
    {
      kind = token_kind::RBracket;
      data = "]";
    } break;
  case ';':
  {
    kind = token_kind::Semi;
    data = ";";
  } break;
  case ':':
  {
    kind = token_kind::Colon;
    data = ":";
  } break;
  case '\\':
  {
    kind = token_kind::Backslash;
    data = "\\";
  } break;
  case '.':
  {
    kind = token_kind::Dot;
    data = ".";
  } break;
  case '(':
  {
    kind = token_kind::LParen;
    data = "(";
  } break;
  case ')':
  {
    kind = token_kind::RParen;
    data = ")";
  } break;
  case ',':
  {
    kind = token_kind::Comma;
    data = ",";
  } break;
  case EOF:
      kind = token_kind::EndOfFile;
      data = "EOF";
    break;
  }
  return token(kind, data, {module, beg_col + 1, beg_row, col + 1, row + 1});
}

ass::token asm_reader::gett()
{
restart_get:
  symbol data("");
  ass::token_kind kind = ass::token_kind::Undef;
  op_code opc = op_code::UNKNOWN;

  char ch = getc();
  std::size_t beg_row = row;
  std::size_t beg_col = col - 1;

  bool starts_with_zero = false;
  switch(ch)
  {
  default:
    if(!std::iscntrl(ch) && !std::isspace(ch))
    {
      // Optimistically allow any kind of identifier to allow for unicode
      std::string name;
      name.push_back(ch);
      while(col < linebuf.size())
      {
        // don't use get<char>() here, identifiers are not connected with a '\n'
        ch = linebuf[col++];

        // we look up in the operator map only for the single char. otherwise we wont parse y:= not correctly for example
        std::string ch_s;
        ch_s.push_back(ch);
        // break if we hit whitespace (or other control chars) or any other operator symbol char
        if(std::iscntrl(ch) || std::isspace(ch))
        {
          col--;
          break;
        }
        name.push_back(ch);
      }
      data = symbol(name);
      if(auto it = ass::keyword_map.find(name); it != ass::keyword_map.end())
      { kind = ass::token_kind::Opcode; opc = it->second; }
      else
      {
        // could also be a register
        if(!name.empty() && name[0] == '$')
        {
          try
          {
            std::size_t regnum = std::stoi(std::string(std::next(name.begin()), name.end()));

            if(regnum > vm::register_count - 1) // we only have so many registers
              kind = ass::token_kind::Undef;

            data = symbol(std::string(std::next(name.begin()), name.end()));
            kind = ass::token_kind::Register;
          }
          catch(...)
          { kind = ass::token_kind::Undef; }
        }
        // could also be a labeldef
        else if(name.size() > 1 && name.back() == ':')
        {
          data = symbol(std::string(std::next(name.begin()), name.end()));
          kind = ass::token_kind::LabelDef;
        }
        // or labeluse
        else if(!name.empty() && name.front() == '%')
        {
          data = symbol(std::string(std::next(name.begin()), name.end()));
          kind = ass::token_kind::LabelUse;
        }
        // or directive    i.e. .text/.code, .data
        else if(!name.empty() && name.front() == '.')
        {
          data = symbol(std::string(std::next(name.begin()), name.end()));
          kind = ass::token_kind::Directive;
        }
        else
          kind = ass::token_kind::Undef;
      }
    }
    else
    {
      assert(false && "Control or space character leaked into get<token>!");
    }
    break;

  case '0': starts_with_zero = true; case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    {
      // readin until there is no number anymore. Disallow 000840
      std::string name;
      name.push_back(ch);
      bool emit_not_a_number_error = false;
      bool emit_starts_with_zero_error = false;
      while(col < linebuf.size())
      {
        // don't use get<char>() here, numbers are not connected with a '\n'
        ch = linebuf[col++];

        // we look up in the operator map only for the single char. otherwise we wont parse 2; not correctly for example
        // This could give problems with floaing point numbers since 2.3 will now be parsed as "2" "." "3" so to literals and a point
        std::string ch_s;
        ch_s.push_back(ch);
        // break if we hit whitespace (or other control chars) or any other token char
        if(std::iscntrl(ch) || std::isspace(ch))
        {
          col--;
          break;
        }
        if(!std::isdigit(ch))
          emit_not_a_number_error = true;
        if(starts_with_zero && !emit_not_a_number_error)
          emit_starts_with_zero_error = true;
        name.push_back(ch);
      }
      if(emit_starts_with_zero_error)
      {
        diagnostic <<= diagnostic_db::parser::leading_zeros(source_range {module, beg_col + 1, beg_row, col + 1, row + 1 }, name);
        goto restart_get; // <- SO WHAT
      }
      if(emit_not_a_number_error)
      {
        diagnostic <<= diagnostic_db::parser::no_number(source_range {module, beg_col + 1, beg_row, col + 1, row + 1 }, name);
        goto restart_get;
      }
      kind = ass::token_kind::ImmediateValue;
      data = symbol(name);
    } break;

  case EOF:
      kind = ass::token_kind::EndOfFile;
      opc = op_code::HALT;
      data = "EOF";
    break;
  }
  return ass::token(kind, opc, data, {module, beg_col + 1, beg_row, col + 1, row + 1});
}

