#include "types.hpp"
#include <diagnostic.hpp>
#include <diagnostic_db.hpp>
#include <stream_lookup.hpp>
#include <fixit_info.hpp>

#include <reader.hpp>
#include <token.hpp>
#include <vm.hpp>

#include <tsl/robin_map.h>

#include <string_view>
#include <cassert>
#include <istream>
#include <sstream>
#include <memory>
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
  "case"sv,
  "TOP"sv,
  "BOT"sv,
  "Type"sv,
  "Kind"sv,
  "Prop"sv,
  "type"sv,
  "data"sv
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

auto token_precedence_map = tsl::robin_map<token_kind, int>( {
  {token_kind::Colon, 1},
  {token_kind::Dot, 7},
  {token_kind::Plus, 5},
  {token_kind::Minus, 5},
  {token_kind::Asterisk, 6},
  {token_kind::LParen, 65535},
  {token_kind::LBrace, 65535},
  {token_kind::Identifier, 6}, //65535},
  {token_kind::LiteralNumber, 65535},
});

constexpr static bool isprint(unsigned char c)
{ return (' ' <= c && c <= '~'); }

static bool is_syntactically_callable(hx_ir& sc, std::size_t prefix)
{
  auto pref = sc.nodes.back().kinds[prefix];

  return !(pref == IRNodeKind::binary_exp
        || pref == IRNodeKind::literal
        || pref == IRNodeKind::unit
        || pref == IRNodeKind::tuple
        || pref == IRNodeKind::top
        || pref == IRNodeKind::bot);
}

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
  /*
  case '_':
  {
    kind = token_kind::Underscore;
    data = "_";
  } break;
  */
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

void hx_reader::consume()
{
  old = current;
  current = next_toks[0];

  for(std::size_t i = 0, j = 1; j < next_toks.size(); ++i, ++j)
    std::swap(next_toks[i], next_toks[j]);

  next_toks.back() = gett();
}

#define expect(kind__, f) (([this](){ if(current.kind != static_cast<token_kind>(kind__)) {\
    diagnostic <<= f(current.loc, current.data.get_string()); \
    consume(); \
    return false; \
    } consume(); return true; })())

#define accept(kind__) (([this](){ if(current.kind != static_cast<token_kind>(kind__)) {\
    return false; \
    } consume(); return true; })())

std::size_t hx_reader::mk_error()
{ return error_ref; }

// nat
std::size_t hx_reader::parse_literal()
{
  // Only have numbers as literals for now
  if(!expect(token_kind::LiteralNumber, diagnostic_db::parser::literal_expected))
    return mk_error();
  return IRTags::literal.make_node(global_scope.nodes.back(), IRData { old.data }, IRDebugData { old.loc });
}

// id  or special identifier "_" if pattern parsing is enabled
std::size_t hx_reader::parse_identifier()
{
  if(!expect(token_kind::Identifier, diagnostic_db::parser::identifier_expected))
  {
    fixits_stack.back().changes.emplace_back(old.loc.snd_proj(), nlohmann::json { {"what", old.loc},
                                                                       {"how", "id"} });
    return mk_error();
  }
  if(!parsing_pattern && old.data == symbol("_"))
  {
    diagnostic <<= diagnostic_db::parser::invalid_identifier_for_non_pattern(old.loc, old.data.get_string());

    fixits_stack.back().changes.emplace_back(old.loc.snd_proj(), nlohmann::json { {"what", old.loc},
                                                                       {"how", "id"} });
    return mk_error();
  }

  auto id = old.data;
  auto present = std::find_if(scoping_ctx.binder_stack.rbegin(), scoping_ctx.binder_stack.rend(),
        [&id](auto x) { return x.first == id; });

  if(present != scoping_ctx.binder_stack.rend())
  {
    if(id == symbol("_"))
    {
      // Do not reference
      return IRTags::identifier.make_node(global_scope.nodes.back(),
                                          IRData { old.data },
                                          IRDebugData { old.loc });
    }
    else
    {
      return IRTags::identifier.make_node(global_scope.nodes.back(),
                                          IRData { old.data, nullptr, present->second },
                                          IRDebugData { old.loc });
    }
  }
  if(!global_scope.nodes.back().free_variable_roots.contains(id))
  {
    // add free variable to free variables list
    global_scope.nodes.back().
      free_variable_roots[id].refs.
      push_back(global_scope.nodes.back().kinds.size());

    auto& b = global_scope.nodes.back().free_variable_roots[id].potentially_bounded_ref;
    if(scoping_ctx.is_binding)
      b = global_scope.nodes.back().kinds.size();
  }
  return IRTags::identifier.make_node(global_scope.nodes.back(),
                                      IRData { old.data }, IRDebugData { old.loc });
}

// e := e1 e2
std::size_t hx_reader::parse_app(std::size_t lhs)
{
  auto& b = global_scope.nodes.back();

  b.kinds.insert(b.kinds.begin() + lhs, IRNodeKind::app);
  b.data.insert(b.data.begin() + lhs, IRData { "" });
  // TODO add debug
  b.references.insert(b.references.begin() + lhs, 2);

  //push();
  parse_expression(9);
  return lhs;
}

// e := e1 `.` e2
std::size_t hx_reader::parse_access(std::size_t lhs)
{
  if(!expect('.', diagnostic_db::parser::access_expects_dot))
    return mk_error();
  auto& b = global_scope.nodes.back();

  b.kinds.insert(b.kinds.begin() + lhs, IRNodeKind::access);
  b.data.insert(b.data.begin() + lhs, IRData { old.data });
  // TODO add debug
  b.references.insert(b.references.begin() + lhs, 2);
  //push();
  parse_expression();
  return lhs;
}

// e := `\\` id `.` e       id can be _ to simply ignore the argument. Note that `\\` is a single backslash
//   For types we have a Pi type
// e := `\\` `(` x `:` A  `)` `.` B        where both A,B are types.
//                                         parantheses are needed to distinguish between pi and tuple access
std::size_t hx_reader::parse_lambda()
{
  auto lam_tok = current;
  if(!expect('\\', diagnostic_db::parser::lambda_expects_lambda))
    return mk_error();
  std::size_t to_ret = IRTags::lambda.make_node(global_scope.nodes.back(), IRData { lam_tok.data },
                                                IRDebugData { lam_tok.loc }, 2);
  parsing_pattern = true;
  auto param = parse_identifier();
  parsing_pattern = false;
  auto psymb = old.data;

  if(!expect('.', diagnostic_db::parser::lambda_expects_dot))
    return mk_error();
  scoping_ctx.binder_stack.emplace_back(psymb, global_scope.nodes.back().kinds.size() - 1);
  auto expr = parse_expression();
  scoping_ctx.binder_stack.pop_back();

  lam_tok.loc += old.loc;

  return to_ret;
}

// e := `case` e `[` p1 `=>` e1 `|` ... `|` pn `=>` en `]`
std::size_t hx_reader::parse_case()
{
  std::size_t to_ret = IRTags::pattern_matcher.make_node(global_scope.nodes.back(), IRData { current.data },
                                           IRDebugData { current.loc }, 0);
  auto keyword = current;
  if(!expect(token_kind::Keyword, diagnostic_db::parser::case_expects_keyword))
    return mk_error();

  auto to_match = parse_expression();

  if(!expect('[', diagnostic_db::parser::case_expects_lbracket))
    return mk_error();

  std::vector<std::size_t> patterns;
  auto mat = parse_match();
  patterns.emplace_back(mat);
  while(current.kind != token_kind::RBracket && current.kind != token_kind::EndOfFile)
  {
    if(!expect('|', diagnostic_db::parser::case_expects_pipe))
      return mk_error();

    mat = parse_match();
    patterns.emplace_back(mat);
  }
  if(!expect(']', diagnostic_db::parser::case_expects_rbracket))
    return mk_error();

  global_scope.nodes.back().references[to_ret] = 1 + patterns.size();

  return to_ret;
}

// effectively just an expression that allows the `_` to occur
std::size_t hx_reader::parse_pattern()
{
  parsing_pattern = true;
  // check if we have the placeholder
  if(current.kind == token_kind::Identifier && current.data == symbol("_"))
  {
    auto id = parse_identifier();
    auto to_return = IRTags::pattern.make_node(global_scope.nodes.back(), IRData { old.data }, IRDebugData { old.loc }, 1);
    parsing_pattern = false;

    return to_return;
  }
  auto vold = current;

  std::size_t to_ret = IRTags::pattern.make_node(global_scope.nodes.back(), IRData { vold.data }, IRDebugData { vold.loc }, 1);

  parsing_pattern = true;
  scoping_ctx.is_binding = true;
  auto expr = parse_expression();
  scoping_ctx.is_binding = false;
  parsing_pattern = false;

  return to_ret;
}

// p `=>` e
std::size_t hx_reader::parse_match()
{
  std::size_t to_ret = IRTags::match.make_node(global_scope.nodes.back(), IRData { "->" }, IRDebugData { current.loc }, 2);

  auto pat = parse_pattern();

  auto arrow = current;
  if(!expect(token_kind::Doublearrow, diagnostic_db::parser::pattern_expects_double_arrow))
    return mk_error();

  auto expr = parse_expression();

  // TODO: change debug data to correct location

  return to_ret;
}

// e := Kind
std::size_t hx_reader::parse_Kind()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Kind))
    return mk_error();
  return IRTags::Kind.make_node(global_scope.nodes.back(), IRData { old.data }, IRDebugData { old.loc });
}

// e := Type
std::size_t hx_reader::parse_Type()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Type))
    return mk_error();
  return IRTags::Type.make_node(global_scope.nodes.back(), IRData { old.data }, IRDebugData { old.loc });
}

// e := Prop
std::size_t hx_reader::parse_Prop()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Prop))
    return mk_error();
  return IRTags::Type.make_node(global_scope.nodes.back(), IRData { old.data }, IRDebugData { old.loc });
}

// e := top
std::size_t hx_reader::parse_top()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_top))
    return mk_error();
  return IRTags::top.make_node(global_scope.nodes.back(), IRData { old.data }, IRDebugData { old.loc });
}

// e := bot
std::size_t hx_reader::parse_bot()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_bot))
    return mk_error();
  return IRTags::bot.make_node(global_scope.nodes.back(), IRData { old.data }, IRDebugData { old.loc });
}

// block := `{` e1 `;` ... `;` en `}`
std::size_t hx_reader::parse_block()
{
  auto tmp = current;
  std::size_t to_ret = IRTags::block.make_node(global_scope.nodes.back(), IRData { tmp.data }, IRDebugData { tmp.loc }, 0);

  // ensure we see an opening brace
  if(!expect('{', diagnostic_db::parser::block_expects_lbrace))
    return mk_error();

  auto expr = parse_expression(0);

  std::vector<std::size_t> v;
  v.emplace_back(expr);
  while(current.kind != token_kind::RBrace && current.kind != token_kind::EndOfFile)
  {
    expect(';', diagnostic_db::parser::block_expects_semicolon);

    expr = parse_expression(0);

    v.emplace_back(expr);
  }
  // ensure that we see a closing brace
  if(!expect('}', diagnostic_db::parser::block_expects_rbrace))
    return mk_error();

  global_scope.nodes.back().references[to_ret] = v.size();
  return to_ret;
}

std::shared_ptr<type_base> hx_reader::parse_kind()
{
  switch(current.data.get_hash())
  {
  default: {
    diagnostic <<= diagnostic_db::parser::not_a_sort(current.loc, current.data.get_string());
    return nullptr;
  }

  case hash_string("Type"): {
    if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Type))
      return nullptr;
    return global_scope.types[global_scope.types.Type_sort_idx];
  }
  case hash_string("Kind"): {
    if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Kind))
      return nullptr;
    return global_scope.types[global_scope.types.Kind_sort_idx];
  }
  case hash_string("Prop"): {
    if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Prop))
      return nullptr;
    return global_scope.types[global_scope.types.Prop_sort_idx];
  }
  }
  assert(false && "unreachable");
  return nullptr;
}

// type := `\` `(` id `:` type `)` `.` type
std::shared_ptr<type_base> hx_reader::parse_pi()
{
  auto lam_tok = current;
  if(!expect('\\', diagnostic_db::parser::lambda_expects_lambda))
    return nullptr;
  
  if(!expect('(', diagnostic_db::parser::pi_expects_lparen))
    return nullptr;

  if(!expect(token_kind::Identifier, diagnostic_db::parser::identifier_expected))
  {
    fixits_stack.back().changes.emplace_back(old.loc.snd_proj(), nlohmann::json { {"what", old.loc},
                                                                       {"how", "id"} });
    return nullptr;
  }
  symbol arg_name = old.data;
  parsing_pattern = false;

  if(!expect(':', diagnostic_db::parser::pi_requires_explicit_domain))
    return nullptr;

  // parse the domain type
  auto domain_type = parse_type();

  auto domain = std::make_shared<IdTypeBox>(arg_name, domain_type);

  if(!expect(')', diagnostic_db::parser::pi_expects_rparen))
    return nullptr;
  
  if(!expect('.', diagnostic_db::parser::lambda_expects_dot))
    return nullptr;
  auto body = parse_type();

  lam_tok.loc += old.loc;
  assert(domain != nullptr && "Domain must be valid here!");
  return std::make_shared<pi_type>(domain, body);
}

std::shared_ptr<type_base> hx_reader::parse_parenthesized_type()
{
  if(!expect('(', diagnostic_db::parser::type_expects_lparen))
    return nullptr;

  auto typ = parse_type();

  if(!expect(')', diagnostic_db::parser::type_expects_lparen))
    return nullptr;
  return typ;
}

std::shared_ptr<type_base> hx_reader::parse_type_prefix()
{
  switch(current.kind)
  {
  case token_kind::Keyword:    return parse_kind();
  case token_kind::Backslash:  return parse_pi();
  case token_kind::Identifier: return parse_type_identifier();
  case token_kind::LParen:     return parse_parenthesized_type();
      //TODO
  default: ;
  }
  assert(false && "Case not handled.");
  return nullptr;
}

std::shared_ptr<type_base> hx_reader::parse_type_identifier()
{
  if(!expect(token_kind::Identifier, diagnostic_db::parser::identifier_expected))
  {
    fixits_stack.back().changes.emplace_back(old.loc.snd_proj(), nlohmann::json { {"what", old.loc},
                                                                       {"how", "id"} });
    return nullptr;
  }
  if(!parsing_pattern && old.data == symbol("_"))
  {
    diagnostic <<= diagnostic_db::parser::invalid_identifier_for_non_pattern(old.loc, old.data.get_string());

    fixits_stack.back().changes.emplace_back(old.loc.snd_proj(), nlohmann::json { {"what", old.loc},
                                                                       {"how", "id"} });
    return nullptr;
  }
  return std::make_shared<type_or_value_ref>(old.data);
}

std::shared_ptr<type_base> hx_reader::parse_type_app(std::shared_ptr<type_base> lhs)
{
  return std::make_shared<application>(lhs, parse_type());
}

std::shared_ptr<type_base> hx_reader::parse_type(int precedence)
{
  auto pref = parse_type_prefix();

  if(pref == nullptr)
    return nullptr;

  while(precedence < this->precedence())
  {
    switch(current.kind)
    {
    // TODO: add all other expressions. Somehow, we should get rid of the duplicate code and mix things up
    // only stop if the expression cannot be expanded further
      case token_kind::Semi:
      case token_kind::RParen:
      case token_kind::RBracket:
      case token_kind::RBrace:
      case token_kind::Pipe:
      case token_kind::Comma:
      case token_kind::Colon:
      case token_kind::LBracket:
      case token_kind::Doublearrow:
      {
        return pref;
      } break;

      default:
      {
        // NOTE: We assume that old.kind is a thing that is surely callable
        pref = parse_type_app(pref);
      } break;
    }
  }
  return pref;
}

// s := `data` name ( `(` id `:` type `)` )* `:` type `;`
std::size_t hx_reader::parse_data_ctor()
{
  auto old_loc = current.loc;
  std::size_t to_ret = IRTags::assign_data.make_node(global_scope.nodes.back(), IRData { "" },
                                        IRDebugData { old_loc }, 1);
  if(!expect(token_kind::Keyword, diagnostic_db::parser::type_keyword_expected))
    return mk_error();

  scoping_ctx.is_binding = true;
  auto type_name = parse_identifier();
  scoping_ctx.is_binding = false;
  auto type_name_id = old.data;

  std::vector<IdTypeBox> params; params.reserve(32);
  while(accept('('))
  {
    if(!expect(token_kind::Identifier, diagnostic_db::parser::identifier_expected))
    {
      fixits_stack.back().changes.emplace_back(old.loc.snd_proj(), nlohmann::json { {"what", old.loc},
                                                                         {"how", "id"} });
      return mk_error();
    }
    // symbol("_") is ok
    symbol id = old.data;

    if(!expect(':', diagnostic_db::parser::type_ctor_param_expects_colon))
      return mk_error();

    auto type = parse_type();

    if(!type || !expect(')', diagnostic_db::parser::type_ctor_param_expects_closing_paranthesis))
      return mk_error();

    params.emplace_back(id, type);
  }
  if(!expect(':', diagnostic_db::parser::type_assign_expects_equal))
    return mk_error();

  auto tail = parse_type();

  if(!tail || !expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end))
    return mk_error();

  // build new type
  for(auto it = params.rbegin(); it != params.rend(); ++it)
  {
    tail = std::make_shared<pi_type>(std::make_shared<IdTypeBox>(std::move(*it)), tail);
  }

  fixits_stack.pop_back();
  global_scope.nodes.back().data[to_ret] = IRData { type_name_id, tail };
  global_scope.constructors.back().data.emplace_back(type_name_id, tail);

  global_scope.nodes.back().node_name = std::make_shared<symbol>(type_name_id);

  return to_ret;
}

// s := `type` name ( `(` id `:` type `)` )* `:` Sort `;`
std::size_t hx_reader::parse_type_ctor()
{
  auto old_loc = current.loc;
  if(!expect(token_kind::Keyword, diagnostic_db::parser::type_keyword_expected))
    return mk_error();

  std::size_t to_ret = IRTags::assign_type.make_node(global_scope.nodes.back(), IRData { "" },
                                        IRDebugData { old_loc }, 1);
  scoping_ctx.is_binding = true;
  auto type_name = parse_identifier();
  scoping_ctx.is_binding = false;
  auto type_name_id = old.data;

  std::vector<IdTypeBox> params; params.reserve(32);
  while(accept('('))
  {
    if(!expect(token_kind::Identifier, diagnostic_db::parser::identifier_expected))
    {
      fixits_stack.back().changes.emplace_back(old.loc.snd_proj(), nlohmann::json { {"what", old.loc},
                                                                         {"how", "id"} });
      return mk_error();
    }
    // symbol("_") is ok
    symbol id = old.data;

    if(!expect(':', diagnostic_db::parser::type_ctor_param_expects_colon))
      return mk_error();

    auto type = parse_type();

    if(!type || !expect(')', diagnostic_db::parser::type_ctor_param_expects_closing_paranthesis))
      return mk_error();

    params.emplace_back(id, type);
  }
  if(!expect(':', diagnostic_db::parser::type_assign_expects_equal))
    return mk_error();

  auto tail = parse_type();

  if(!tail || !expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end))
    return mk_error();

  // build new type
  for(auto it = params.rbegin(); it != params.rend(); ++it)
  {
    tail = std::make_shared<pi_type>(std::make_shared<IdTypeBox>(std::move(*it)), tail);
  }

  fixits_stack.pop_back();
  
  global_scope.nodes.back().data[to_ret] = IRData { type_name_id, tail };

  auto p = std::make_shared<type_constructor>(type_name_id, tail);
  global_scope.types.types.emplace_back(p);
  global_scope.constructors.emplace_back(p);

  global_scope.nodes.back().node_name = std::make_shared<symbol>(type_name_id);

  return to_ret;
}


std::size_t hx_reader::parse_constructor()
{
  parsing_constructor = true;
  auto expr = parse_expression();
  parsing_constructor = false;
  return expr;
}

std::size_t hx_reader::parse_assign()
{
  std::size_t to_ret = IRTags::assign.make_node(global_scope.nodes.back(), IRData { current.data }, IRDebugData { current.loc }, 2);

  auto current_source_loc = current.loc;
  scoping_ctx.is_binding = true;
  auto var = parse_identifier();
  scoping_ctx.is_binding = false;
  auto var_symb = old.data;
  if(!expect('=', diagnostic_db::parser::assign_expects_equal))
    return mk_error();
  auto arg = parse_expression();

  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end))
  {
    source_range loc = (arg == error_ref ? source_range{} : global_scope.nodes.back().debug_data[var].loc);

    auto proj = loc.snd_proj();
    // do not want to ignore this token
    loc.column_beg = loc.column_end;
    fixits_stack.back().changes.emplace_back(proj, nlohmann::json { {"what", loc}, {"how", ";"} });

    diagnostic <<= mk_diag::fixit(current_source_loc + loc, fixits_stack.back());

    fixits_stack.pop_back();
    return mk_error();
  }

  fixits_stack.pop_back();

  global_scope.nodes.back().node_name = std::make_shared<symbol>(var_symb);

  return to_ret;
}

std::size_t hx_reader::parse_expr_stmt()
{
  std::size_t to_ret = IRTags::expr_stmt.make_node(global_scope.nodes.back(), IRData { "" }, IRDebugData { current.loc }, 1);
  auto current_source_loc = current.loc;

  auto expr = parse_expression();
  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end))
  {
    source_range loc = (expr == error_ref ? source_range{} : global_scope.nodes.back().debug_data[expr].loc);
    auto proj = loc.snd_proj();

    // do not want to ignore this token
    loc.column_beg = loc.column_end;
    fixits_stack.back().changes.emplace_back(proj, nlohmann::json { {"what", loc}, {"how", ";"} });

    diagnostic <<= mk_diag::fixit(current_source_loc + loc, fixits_stack.back());

    fixits_stack.pop_back();
    return mk_error();
  }
  fixits_stack.pop_back();

  //TODO: fix data
  return to_ret;
}

std::size_t hx_reader::parse_statement()
{
  fixits_stack.emplace_back();
  if(current.kind == token_kind::Keyword && current.data.get_hash() == symbol("type").get_hash())
    return parse_type_ctor();
  else if(current.kind == token_kind::Keyword && current.data.get_hash() == symbol("data").get_hash())
    return parse_data_ctor();
  else if(next_toks[0].kind == token_kind::Equal)
    return parse_assign();
  return parse_expr_stmt();
}

std::size_t hx_reader::parse_keyword()
{
  switch(current.data.get_hash())
  {
  case hash_string("case"): return parse_case();
  case hash_string("TOP"):  return parse_top();
  case hash_string("BOT"):  return parse_bot();
  case hash_string("Type"): return parse_Type();
  case hash_string("Kind"): return parse_Kind();
  case hash_string("Prop"): return parse_Prop();
  }
  assert(false && "bug in lexer, we would not see a keyword token otherwise");
  return mk_error();
}

// e := e1 +-* e2
std::size_t hx_reader::parse_binary(std::size_t left)
{
  assert(!parsing_pattern);
  token op = current;
  auto& b = global_scope.nodes.back();

  b.kinds.insert(b.kinds.begin() + left, IRNodeKind::binary_exp);
  b.data.insert(b.data.begin() + left, IRData { old.data });
  // TODO add debug
  b.references.insert(b.references.begin() + left, 2);

  consume();
  int precedence = token_precedence_map[op.kind];
  //push();
  auto right = parse_expression(precedence);
  return left;
}

// e := `(` e `)` | `(` e1 `,` ... `,` en `)`
std::size_t hx_reader::parse_tuple()
{
  expect('(', diagnostic_db::parser::tuple_or_unit_expr_expect_lparen);
  auto open = old;

  // See if we have a unit
  if(accept(')'))
    return IRTags::unit.make_node(global_scope.nodes.back(), IRData { "()" }, IRDebugData { open.loc });
  
  auto first_expr = parse_expression();

  // See if we have a parenthesized expression
  if(accept(')'))
    return first_expr;

  if(current.kind != token_kind::Comma)
  {
    if(is_syntactically_callable(global_scope, first_expr))
    {
      auto to_ret = parse_app(first_expr);

      if(!expect(')', diagnostic_db::parser::closing_parenthesis_expected))
        return mk_error();
      return parse_app(first_expr);
    }
    else
    {
      expect(',', diagnostic_db::parser::tuple_expects_comma);
      return mk_error();
    }
  }
  std::size_t to_ret = IRTags::tuple.make_node(global_scope.nodes.back(), IRData { open.data }, IRDebugData { open.loc }, 0);

  std::vector<std::size_t> v;
  v.emplace_back(first_expr);
  while(current.kind != token_kind::RParen && current.kind != token_kind::EndOfFile)
  {
    expect(',', diagnostic_db::parser::tuple_expects_comma);

    auto expr = parse_expression(0);
    v.emplace_back(expr);
  }
  // ensure that we see a closing brace
  if(!expect(')', diagnostic_db::parser::tuple_expects_closing_paranthesis))
    return mk_error();
  global_scope.nodes.back().references[to_ret] = v.size();
  return to_ret;
}

// e := identifier, number
std::size_t hx_reader::parse_prefix()
{
  switch(current.kind)
  {
    default:
    {
      // TODO: improve error message depending on current.kind
      diagnostic <<= diagnostic_db::parser::not_a_prefix(current.loc, current.data.get_string());

      return mk_error();
    }

    case token_kind::LiteralNumber:
    {
      if(parsing_constructor)
      {
        diagnostic <<= diagnostic_db::parser::constructor_expected(current.loc, current.data.get_string());
        return mk_error();
      }
      return parse_literal();
    }
    case token_kind::Identifier:
    {
      return parse_identifier();
    }
    case token_kind::LParen:
    {
      return parse_tuple();
    }
    case token_kind::LBrace:
    {
      if(parsing_constructor)
      {
        diagnostic <<= diagnostic_db::parser::constructor_expected(current.loc, current.data.get_string());
        return mk_error();
      }
      return parse_block();
    }
    case token_kind::Keyword:
    {
      if(parsing_constructor)
      {
        diagnostic <<= diagnostic_db::parser::constructor_expected(current.loc, current.data.get_string());
        return mk_error();
      }
      return parse_keyword();
    }
    case token_kind::Backslash:
    {
      if(parsing_constructor)
      {
        diagnostic <<= diagnostic_db::parser::constructor_expected(current.loc, current.data.get_string());
        return mk_error();
      }
      return parse_lambda();
    }
  }
}

// e `:` t
std::size_t hx_reader::parse_type_check(std::size_t left)
{
  if(parsing_type_checking) // can't type check inside a type check
  {
    diagnostic <<= diagnostic_db::parser::type_check_inside_type_check(current.loc);
    return mk_error();
  }
  // We only call this function if we have seen the colon before!
  assert(expect(':', diagnostic_db::parser::unknown_token));
  auto colon = old;

  // We have dependent types, so the type is arbitrary
  // However, any lambda becomes a Pi.
  parsing_type_checking = true;
  auto right = parse_type();
  parsing_type_checking = false;

  if(right == nullptr)
    return -1;

  auto& b = global_scope.nodes.back();

  b.kinds.insert(b.kinds.begin() + left, IRNodeKind::type_check);
  b.data.insert(b.data.begin() + left, IRData { colon.data, right });
  // TODO add debug
  b.references.insert(b.references.begin() + left, 1);
  //push();

  return left;
}

// e
std::size_t hx_reader::parse_expression(int precedence)
{
  auto prefix = parse_prefix();
  
  if(prefix == error_ref)
    return error_ref;

  while(precedence < this->precedence())
  {
    //hold();
    switch(current.kind)
    {
      // only stop if the expression cannot be expanded further
      case token_kind::Semi:
      case token_kind::RParen:
      case token_kind::RBracket:
      case token_kind::RBrace:
      case token_kind::Pipe:
      case token_kind::Comma:
      case token_kind::LBracket:
      case token_kind::Doublearrow:
      {
        //push();
        return prefix;
      } break;

      case token_kind::Colon:
      {
        //push();
        return parse_type_check(prefix);
      } break;

      default:
      {
        // ensure before that prefix is something callable
        if(!is_syntactically_callable(global_scope, prefix))
        {
          // not callable
          //push();
          return prefix;
        }
        prefix = parse_app(prefix);
      } break;

      case token_kind::Dot:
      {
        if(parsing_constructor)
        {
          diagnostic <<= diagnostic_db::parser::constructor_expected(current.loc, current.data.get_string());
          return mk_error();
        }
        prefix = parse_access(prefix);
      } break;

      case token_kind::Plus:
      case token_kind::Minus:
      case token_kind::Asterisk:
      {
        if(parsing_constructor)
        {
          diagnostic <<= diagnostic_db::parser::constructor_expected(current.loc, current.data.get_string());
          return mk_error();
        }
        prefix = parse_binary(prefix);
      } break;
    }
    if(prefix == error_ref)
    {
      // TODO
      return error_ref;
    }
  }

  return prefix;
}

template<>
std::vector<hx_ir> hx_reader::read(std::string_view module)
{
  hx_reader r(module);

  while(r.current.kind != token_kind::EndOfFile)
  {
    r.global_scope.nodes.emplace_back();
    auto stmt = r.parse_statement();

    // if(stmt == error_ref)   please fix it, user
  }

  if(r.global_scope.nodes.empty() && diagnostic.empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(r.current.loc);

  r.global_scope.build_graph();
  return { r.global_scope };
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
  if(toks.empty() && diagnostic.empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(r.current.loc);
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
  return { std::nullopt, std::nullopt, current, {} };
}

ass::instruction asm_reader::parse_labeldef()
{
  return { std::nullopt, current, std::nullopt, {} };
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
  if(instructions.empty() && diagnostic.empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(r.current.loc);
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
  if(instructions.empty() && diagnostic.empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(r.current.loc);
  return instructions;
}

#undef expect
#undef accept

