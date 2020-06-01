#include <reader.hpp>
#include <diagnostics.h>
#include <diagnostic_db.hpp>

#include <tsl/robin_map.h>

#include <sstream>

auto token_precedence_map = tsl::robin_map<token_kind, int>( {
  {token_kind::Colon, 1},
  {token_kind::Dot, 7},
  {token_kind::Plus, 5},
  {token_kind::Minus, 5},
  {token_kind::Asterisk, 6},
  {token_kind::LParen, 65535},
  {token_kind::LBrace, 65535},
  {token_kind::Identifier, 1}, //65535},
  {token_kind::LiteralNumber, 65535},
});

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

ast_ptr hx_reader::mk_error()
{ return error_ref; }

// id  or special identifier "_" if pattern parsing is enabled
ast_ptr hx_reader::parse_identifier()
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
    }
    else
    {
    }
  }
  {
    // add free variable to free variables list
  }
  return nullptr;
}

// e := e1 e2
ast_ptr hx_reader::parse_app(ast_ptr lhs)
{
  auto rhs = parse_expression(65535);
  if(rhs == error_ref)
    return error_ref;

  return nullptr;
}

// e := `\\` id `.` e       id can be _ to simply ignore the argument. Note that `\\` is a single backslash
//   For types we have a Pi type
// e := `\\` `(` x `:` A  `)` `.` B        where both A,B are types.
//                                         parantheses are needed to distinguish between pi and tuple access
ast_ptr hx_reader::parse_lambda()
{
  auto lam_tok = current;
  if(!expect('\\', diagnostic_db::parser::lambda_expects_lambda))
    return mk_error();

  bool type_checking_mode = accept('(');

  parsing_pattern = true;
  auto param = parse_identifier();
  parsing_pattern = false;
  auto psymb = old.data;

  if(accept(':'))
  {
    if(!type_checking_mode)
    {
      // TODO: emit diagnostic
      return mk_error();
    }

    auto typ = parse_expression();

    if(!expect(')', diagnostic_db::parser::closing_parenthesis_expected))
      return mk_error();

    // store type annot
  }

  // TODO: Add type parsing similar to Pi

  if(!expect('.', diagnostic_db::parser::lambda_expects_dot))
    return mk_error();
  scoping_ctx.binder_stack.emplace_back(psymb, param);
  auto expr = parse_expression();
  scoping_ctx.binder_stack.pop_back();

  lam_tok.loc += old.loc;
// TODO:  global_scope.dbg_data[to_ret].loc = lam_tok;

  return nullptr;
}

// e := Kind
ast_ptr hx_reader::parse_Kind()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Kind))
    return mk_error();
  return nullptr;
}

// e := Type
ast_ptr hx_reader::parse_Type()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Type))
    return mk_error();
  return nullptr;
}

// e := Prop
ast_ptr hx_reader::parse_Prop()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Prop))
    return mk_error();
  return nullptr;
}

// s := `data` name ( `(` id `:` type `)` )* `:` type `;`
ast_ptr hx_reader::parse_data_ctor()
{
  auto old_loc = current.loc;
  if(!expect(token_kind::Keyword, diagnostic_db::parser::type_keyword_expected))
    return mk_error();

  scoping_ctx.is_binding = true;
  auto type_name = parse_identifier();
  scoping_ctx.is_binding = false;
  auto type_name_id = old.data;

  if(!expect(':', diagnostic_db::parser::type_assign_expects_equal))
    return mk_error();
  auto tail = parse_expression();
  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end))
    return mk_error();

  return nullptr;
}

// s := `type` name ( `(` id `:` type `)` )* `:` Sort `;`
ast_ptr hx_reader::parse_type_ctor()
{
  auto old_loc = current.loc;
  if(!expect(token_kind::Keyword, diagnostic_db::parser::type_keyword_expected))
    return mk_error();

  scoping_ctx.is_binding = true;
  auto type_name = parse_identifier();
  scoping_ctx.is_binding = false;
  auto type_name_id = old.data;

  if(!expect(':', diagnostic_db::parser::type_assign_expects_equal))
    return mk_error();
  auto tail = parse_expression();

  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end))
    return mk_error();

  return nullptr;
}


ast_ptr hx_reader::parse_assign()
{
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
    source_range loc = {};
    auto proj = loc.snd_proj();

    // do not want to ignore this token
    loc.column_beg = loc.column_end;
    fixits_stack.back().changes.emplace_back(proj, nlohmann::json { {"what", loc}, {"how", ";"} });

    diagnostic <<= mk_diag::fixit(current_source_loc + loc, fixits_stack.back());

    return mk_error();
  }
  return nullptr;
}

ast_ptr hx_reader::parse_expr_stmt()
{
  auto current_source_loc = current.loc;

  auto expr = parse_expression();
  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end))
  {
    source_range loc = source_range {};
    auto proj = loc.snd_proj();

    // do not want to ignore this token
    loc.column_beg = loc.column_end;
    fixits_stack.back().changes.emplace_back(proj, nlohmann::json { {"what", loc}, {"how", ";"} });

    diagnostic <<= mk_diag::fixit(current_source_loc + loc, fixits_stack.back());

    return mk_error();
  }
  //TODO: fix data
  return nullptr;
}

ast_ptr hx_reader::parse_statement()
{
  ast_ptr to_ret = nullptr;
  fixits_stack.emplace_back();
  if(current.kind == token_kind::Keyword && current.data.get_hash() == symbol("type").get_hash())
    to_ret = parse_type_ctor();
  else if(current.kind == token_kind::Keyword && current.data.get_hash() == symbol("data").get_hash())
    to_ret = parse_data_ctor();
  else if(next_toks[0].kind == token_kind::Equal)
    to_ret = parse_assign();
  else
    to_ret = parse_expr_stmt();

  fixits_stack.pop_back();
  return to_ret;
}

ast_ptr hx_reader::parse_keyword()
{
  switch(current.data.get_hash())
  {
  case hash_string("Type"): return parse_Type();
  case hash_string("Kind"): return parse_Kind();
  case hash_string("Prop"): return parse_Prop();
  }
  assert(false && "bug in lexer, we would not see a keyword token otherwise");
  return mk_error();
}

// e := identifier 
ast_ptr hx_reader::parse_prefix()
{
  switch(current.kind)
  {
    default:
    {
      // TODO: improve error message depending on current.kind
      diagnostic <<= diagnostic_db::parser::not_a_prefix(current.loc, current.data.get_string());

      return mk_error();
    }

    case token_kind::LParen:
    {
      if(!expect('(', diagnostic_db::parser::tuple_or_unit_expr_expect_lparen))
        return mk_error();

      auto ex = parse_expression();

      if(!expect(')', diagnostic_db::parser::closing_parenthesis_expected))
        return mk_error();

      return ex;
    }

    case token_kind::Identifier:
    {
      return parse_identifier();
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
ast_ptr hx_reader::parse_type_check(ast_ptr left)
{
  // We only call this function if we have seen the colon before!
  assert(expect(':', diagnostic_db::parser::unknown_token));
  auto colon = old;

  // We have dependent types, so the type is arbitrary
  auto right = parse_expression();

  if(!right)
    return mk_error();

  // store type
  
  return left;
}

// e
ast_ptr hx_reader::parse_expression(int precedence)
{
  auto prefix = parse_prefix();
  
  if(prefix == error_ref)
    return error_ref;

  while(precedence < this->precedence())
  {
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
        return prefix;
      } break;

      case token_kind::Colon:
      {
        return parse_type_check(prefix);
      } break;

      default:
      {
        prefix = parse_app(prefix);
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
std::vector<hx_ast> hx_reader::read(std::string_view module)
{
  hx_reader r(module);

  while(r.current.kind != token_kind::EndOfFile)
  {
    auto stmt = r.parse_statement();

    // if(stmt == error_ref)   please fix it, user
  }

  if(diagnostic.empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(r.current.loc);

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


