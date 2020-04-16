#include <diagnostic.hpp>
#include <diagnostic_db.hpp>
#include <memory>
#include <stream_lookup.hpp>

#include <reader.hpp>
#include <token.hpp>
#include <ast.hpp>
#include <vm.hpp>

#include <tsl/robin_map.h>

#include <string_view>
#include <cassert>
#include <istream>
#include <sstream>
#include <vector>
#include <queue>
#include <map>
#include <set>

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
  "case"sv
});

auto operator_symbols_map = tsl::robin_map<std::string_view, token_kind>({
  {":"sv, token_kind::Colon},
  {";"sv, token_kind::Semi},
  {"+"sv,  token_kind::Plus},
  {"-"sv,  token_kind::Minus},
  {"*"sv,  token_kind::Asterisk},
  {"="sv,  token_kind::Equal},
});

auto token_precedence_map = tsl::robin_map<token_kind, int>( {
  {token_kind::Plus, 1},
  {token_kind::Minus, 1},
  {token_kind::Asterisk, 2},
  {token_kind::LiteralNumber, 6},
  {token_kind::Identifier, 6},
});

constexpr static bool isprint(unsigned char c)
{ return (' ' <= c && c <= '~'); }


base_reader::base_reader(std::string_view module)
  : module(module), linebuf(), is(stream_lookup[module]), col(0), row(1), uses_reader(true)
{  }

base_reader::base_reader(std::istream& stream)
  : module("#TXT#"), linebuf(), is(stream), col(0), row(1)
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
        if(std::iscntrl(ch) || std::isspace(ch) || operator_symbols_map.count(ch_s.c_str()) || keyword_set.count(name.c_str()))
        {
          col--;
          break;
        }
        name.push_back(ch);
      }
      kind = (operator_symbols_map.count(name.c_str()) ? operator_symbols_map[name.c_str()] : token_kind::Identifier);
      kind = (keyword_set.count(name.c_str()) ? token_kind::Keyword : kind);
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
        diagnostic <<= diagnostic_db::parser::leading_zeros(module, beg_row, beg_col + 1, name);
        goto restart_get; // <- SO WHAT
      }
      if(emit_not_a_number_error)
      {
        diagnostic <<= diagnostic_db::parser::no_number(module, beg_row, beg_col + 1, name);
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
  }
  case '_':
  {
    kind = token_kind::Underscore;
    data = "_";
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
    kind = token_kind::Minus;
    data = "-";
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
        diagnostic <<= diagnostic_db::parser::leading_zeros(module, beg_row, beg_col + 1, name);
        goto restart_get; // <- SO WHAT
      }
      if(emit_not_a_number_error)
      {
        diagnostic <<= diagnostic_db::parser::no_number(module, beg_row, beg_col + 1, name);
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

void hx_reader::consume()
{
  old = current;
  current = next_toks[0];

  for(std::size_t i = 0, j = 1; j < next_toks.size(); ++i, ++j)
    std::swap(next_toks[i], next_toks[j]);

  next_toks.back() = gett();
}

#define expect(kind__, f) ([this](){ if(current.kind != static_cast<token_kind>(kind__)) {\
    diagnostic <<= f(module, current.loc.row_beg, current.loc.column_beg + 1, current.data.get_string()); \
    consume(); \
    return false; \
    } consume(); return true; })()

#define accept(kind__, f) ([this](){ if(current.kind != static_cast<token_kind>(kind__)) {\
    return false; \
    } consume(); return true; })()

error hx_reader::mk_error()
{ return ast_tags::error.make_node(old); }

maybe_expr hx_reader::parse_literal()
{
  // Only have numbers as literals for now
  if(!expect(token_kind::LiteralNumber, diagnostic_db::parser::literal_expected))
    return mk_error();
  return ast_tags::literal.make_node(old);
}

maybe_expr hx_reader::parse_identifier()
{
  if(!expect(token_kind::Identifier, diagnostic_db::parser::identifier_expected))
    return mk_error();
  return ast_tags::identifier.make_node(old);
}

maybe_stmt hx_reader::parse_block()
{
  auto tmp = current;

  // ensure we see an opening brace
  if(!expect('{', diagnostic_db::parser::block_expects_lbrace))
    return mk_error();

  std::vector<maybe_stmt> v;
  while(current.kind != token_kind::RBrace && current.kind != token_kind::EndOfFile)
  {
    // just propagate error nodes
    auto stmt = parse_statement();
    if(std::holds_alternative<stmt_type>(stmt))
      v.emplace_back(std::move(std::get<stmt_type>(stmt)));
    else 
      v.emplace_back(std::move(std::get<error>(stmt)));
  }
  // ensure that we see a closing brace
  if(!expect('}', diagnostic_db::parser::block_expects_rbrace))
    return mk_error();

  return std::move(ast_tags::block.make_node(tmp, std::move(v)));
}

maybe_stmt hx_reader::parse_statement()
{
  return nullptr;
}

maybe_expr hx_reader::parse_prefix() // operator
{
  switch(current.kind)
  {
    default:
    {
      diagnostic <<= diagnostic_db::parser::unknown_token(module, current.loc.row_beg, current.loc.column_beg + 1, current.data.get_string());

      return mk_error();
    }

    case token_kind::LiteralNumber:
    {
      return std::move(parse_literal());
    }
    case token_kind::Identifier:
    {
      return std::move(parse_identifier());
    }
  }
}

exp_type hx_reader::parse_binary(maybe_expr left)
{
  token op = current;
  consume();
  int precedence = token_precedence_map[op.kind];
  auto right = parse_expression(precedence);

  return ast_tags::binary_exp.make_node(std::move(left), op, std::move(right));
}

maybe_expr hx_reader::parse_expression(int precedence)
{
  auto prefix = parse_prefix();
  
  if(std::holds_alternative<error>(prefix))
    return prefix;

  exp_type infix;
  while(precedence < this->precedence())
  {
    switch(current.kind)
    {
      default: return prefix;

      case token_kind::Plus:
      case token_kind::Minus:
      case token_kind::Asterisk:
      {
        prefix = parse_binary(std::move(prefix));
        if(std::holds_alternative<error>(prefix))
        {
          return prefix;
        }
      }
    }
  }

  return prefix;
}

template<>
std::vector<ast_type> hx_reader::read(std::string_view module)
{
  hx_reader r(module);

  std::vector<ast_type> ast;
  return ast;
}

template<>
std::vector<token> hx_reader::read(std::string_view module)
{
  hx_reader r(module);

  std::vector<token> toks;
  while(r.current.kind != token_kind::EndOfFile)
  {
    toks.push_back(r.current);
    r.consume();
  }
  if(toks.empty() && diagnostic.get_all().empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(module, r.current.loc.row_beg, r.current.loc.column_beg + 1);
  return toks;
}

int hx_reader::precedence() {
  token_kind prec = current.kind;
  if (prec == token_kind::Undef || prec == token_kind::EndOfFile || prec == token_kind::Semi)
    return 0;
  return token_precedence_map[prec];
}


void asm_reader::consume()
{
  old = current;
  current = next_toks[0];

  for(std::size_t i = 0, j = 1; j < next_toks.size(); ++i, ++j)
    std::swap(next_toks[i], next_toks[j]);

  next_toks.back() = gett();
}

ass::instruction asm_reader::parse_op(op_code opc)
{
  auto super_old = current;

  std::vector<ass::token> args;
  switch(current.opc)
  {
  default: // No arg
    break;

    // Single arg
  case op_code::JMP:
  case op_code::JMPREL:
  case op_code::JMP_CMP:
  case op_code::JMP_NCMP:
  case op_code::ALLOC:
  case op_code::INC:
  case op_code::DEC:
    {
    consume();
    args.push_back(current);   // TODO: add error detection
    } break;

    // Two args
  case op_code::LOAD:
  case op_code::EQUAL:
  case op_code::GREATER:
  case op_code::LESS:
  case op_code::GREATER_EQUAL:
  case op_code::LESS_EQUAL:
  case op_code::SHIFT_LEFT:
  case op_code::SHIFT_RIGHT:
    {
    consume();
    args.push_back(current);
    consume();
    args.push_back(current);
    } break;
    
    // Three args
  case op_code::ADD:
  case op_code::SUB:
  case op_code::MUL:
  case op_code::DIV:
    {
    consume();
    args.push_back(current);
    consume();
    args.push_back(current);
    consume();
    args.push_back(current);
    } break;
  }
  // get to next tok
  consume();
  return { super_old.opc, std::nullopt, std::nullopt, args };
}

ass::instruction asm_reader::parse_directive()
{
  return { std::nullopt, std::nullopt, current };
}

ass::instruction asm_reader::parse_labeldef()
{
  return { std::nullopt, current, std::nullopt };
}

template<>
std::vector<ass::instruction> asm_reader::read(std::string_view module)
{
  asm_reader r(module);

  std::vector<ass::instruction> instructions;
  while(r.current.kind != ass::token_kind::EndOfFile)
  {
    switch(r.current.kind)
    {
    default:
      // error
      break;
    case ass::token_kind::Opcode:
      {
        instructions.emplace_back(r.parse_op(r.current.opc));
      } break;
    case ass::token_kind::Directive:
      {
        instructions.emplace_back(r.parse_directive());
      } break;
    case ass::token_kind::LabelDef:
      {
        instructions.emplace_back(r.parse_labeldef());
      }
    }
  }
  if(instructions.empty() && diagnostic.get_all().empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(r.module, r.current.loc.row_beg, r.current.loc.column_beg + 1);
  return instructions;
}

template<>
std::vector<ass::instruction> asm_reader::read_text(const std::string& text)
{
  std::stringstream ss(text);
  asm_reader r(ss);

  std::vector<ass::instruction> instructions;
  while(r.current.kind != ass::token_kind::EndOfFile)
  {
    switch(r.current.kind)
    {
    default:
      // error
      r.consume();
      break;
    case ass::token_kind::Opcode:
      {
        instructions.emplace_back(r.parse_op(r.current.opc));
      } break;
    }
  }
  if(instructions.empty() && diagnostic.get_all().empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(r.module, r.current.loc.row_beg, r.current.loc.column_beg + 1);
  return instructions;
}

#undef expect
#undef accept

