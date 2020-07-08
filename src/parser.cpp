#include <reader.hpp>
#include <diagnostic.hpp>
#include <diagnostic_db.hpp>

#include <tsl/robin_map.h>

#include <sstream>

const ast_ptr hx_reader::error_ref = nullptr;

auto token_precedence_map = tsl::robin_map<token_kind, int>( {
  {token_kind::Arrow, 1},
  {token_kind::Plus, 5},
  {token_kind::Minus, 5},
  {token_kind::Asterisk, 6},
  {token_kind::Colon, 7},
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

ast_ptr hx_reader::parse_directive()
{
  assert(expect('#', diagnostic_db::parser::directive_expects_hashtag));

  parse_identifier();
  auto symb = old.data;
  auto oldloc = old.loc;

  auto type = parse_Type();

  bool error = false;

  bool implicit = false;
  if(symb == symbol("Implicit"))
    implicit = true;
  else if(symb == symbol("Explicit"))
    implicit = false;
  else
  {
    diagnostic <<= diagnostic_db::parser::implicit_or_explicit_expected(oldloc, symb.get_string());
    error = true;
  }
  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end) || error)
    return mk_error();
  return std::make_shared<directive>(implicit);
}

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

  if(present != scoping_ctx.binder_stack.rend()) // && !scoping_ctx.is_binding)
  {
    if(id == symbol("_"))
    {
      // Do not reference
      return std::make_shared<identifier>(id);
    }
    else
    {
      if(scoping_ctx.disallow_recursion == present->second)
      {
        diagnostic <<= diagnostic_db::sema::disallow_recursion(old.loc, old.data.get_string());
        return mk_error();
      }

      // literally use the binding as reference
      return present->second;
    }
  }
  else
  {
    auto to_ret = std::make_shared<identifier>(id); // <- free variable
    if(scoping_ctx.is_binding)
      scoping_ctx.binder_stack.emplace_back(id, to_ret);
    return to_ret;
  }
}

// e := e1 e2
ast_ptr hx_reader::parse_app(ast_ptr lhs)
{
  auto rhs = parse_expression(65535);
  if(rhs == error_ref)
    return error_ref;

  return std::make_shared<app>(lhs, rhs);
}

// e := `\\` id `.` e       id can be _ to simply ignore the argument. Note that `\\` is a single backslash
//   For types we have a Pi type
// e := `\\` `(` x `:` A  `)` `.` B        where both A,B are types.
//                                         parantheses are needed to distinguish between pi and tuple access
ast_ptr hx_reader::parse_lambda()
{
  bool error = false;
  if(current.kind == token_kind::Identifier)
  {
    /// Shorthand for \_:A.B
    // A -> B
    parsing_pattern = true;
    auto param = parse_identifier();
    parsing_pattern = false;
    auto psymb = old.data;

    if(!expect(token_kind::Arrow, diagnostic_db::parser::lambda_expects_arrow) || param == error_ref)
      error = true;

    // No scoping, the argument implicitly is "_" so there is nothing to bind!
    auto expr = parse_expression();
    if(expr == error_ref)
      error = true;

    if(error)
      return mk_error();

    auto id = std::make_shared<identifier>("_");
    id->annot = param;
    return std::make_shared<lambda>(id, expr);
  }
  // We require a lambda
  auto lam_tok = current;
  if(!expect('\\', diagnostic_db::parser::lambda_expects_lambda))
    error = true;

  bool type_checking_mode = accept('(');
  std::size_t parentheses = 0;
  while(accept('('))
    parentheses++;

  parsing_pattern = true;
  auto param = parse_identifier();
  parsing_pattern = false;
  auto psymb = old.data;

  if(param == error_ref)
    error = true;

  while(parentheses-- > 0)
  {
    if(!expect(')', diagnostic_db::parser::closing_parenthesis_expected))
    {
      error = true;
      break;
    }
  }

  if(accept(':'))
  {
    if(!type_checking_mode)
    {
      // TODO: emit diagnostic
      error = true;
    }

    auto typ = parse_expression();

    if(!expect(')', diagnostic_db::parser::closing_parenthesis_expected) || typ == error_ref)
      error = true;

    param->annot = typ;
  }
  if(!expect('.', diagnostic_db::parser::lambda_expects_dot))
    error = true;
  scoping_ctx.binder_stack.emplace_back(psymb, param);
  auto expr = parse_expression();
  scoping_ctx.binder_stack.pop_back();

  if(expr == error_ref)
    error = true;

  if(error)
    return mk_error();

  lam_tok.loc += old.loc;
// TODO:  global_scope.dbg_data[to_ret].loc = lam_tok;
  return std::make_shared<lambda>(param, expr);
}

// s := id (a : type)...(z : type) := e ;
ast_ptr hx_reader::parse_function()
{
  auto id = parse_identifier();
  auto fn_name = old.data;

  bool error = id == error_ref;

  std::vector<ast_ptr> params;
  if(!expect('(', diagnostic_db::parser::type_expects_lparen))
    error = true;
  do {
    parsing_pattern = true;
    auto name = parse_identifier();
    parsing_pattern = false;
    auto oldname = old.data;

    if(!expect(':', diagnostic_db::parser::lambda_param_type_decl_expects_colon))
      error = true;

    auto typ = parse_expression();

    name->annot = typ;
    params.emplace_back(name);

    scoping_ctx.binder_stack.emplace_back(oldname, name);
    if(!expect(')', diagnostic_db::parser::closing_parenthesis_expected))
      error = true;
  } while(accept('('));

  if(!expect(token_kind::ColonEqual, diagnostic_db::parser::function_expects_colon_eq))
    error = true;

  auto expr = parse_expression();
  for(auto& x : params)
    scoping_ctx.binder_stack.pop_back();

  if(error || expr == error_ref)
    return mk_error();
  lambda::ptr lam = std::make_shared<lambda>(params.back(), expr, fn_name);

  for(auto it = params.rbegin() + 1; it != params.rend(); ++it)
    lam = std::make_shared<lambda>(*it, lam, fn_name);
  return lam;
}

// e := Kind
ast_ptr hx_reader::parse_Kind()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Kind))
    return mk_error();
  return std::make_shared<kind>();
}

// e := Type
ast_ptr hx_reader::parse_Type()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Type))
    return mk_error();
  return std::make_shared<type>();
}

// e := Prop
ast_ptr hx_reader::parse_Prop()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Prop))
    return mk_error();
  return std::make_shared<prop>();
}

// s := `data` name ( `(` id `:` type `)` )* `:` type `;`
ast_ptr hx_reader::parse_data_ctor()
{
  auto old_loc = current.loc;
  bool error = false;
  if(!expect(token_kind::Keyword, diagnostic_db::parser::type_keyword_expected))
    error = false;

  scoping_ctx.is_binding = true;
  auto type_name = parse_identifier();
  scoping_ctx.is_binding = false;
  auto type_name_id = old.data;

  if(!expect(':', diagnostic_db::parser::type_assign_expects_equal) || type_name == error_ref)
    error = true;

  scoping_ctx.disallow_recursion = type_name;
  auto tail = parse_expression();
  scoping_ctx.disallow_recursion = nullptr;

  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end) || tail == error_ref)
    error = true;

  if(error)
    return mk_error();

  scoping_ctx.binder_stack.emplace_back(type_name_id, type_name);
  
  type_name->annot = tail;
  return std::make_shared<assign_data>(type_name, tail);
}

// s := `type` name ( `(` id `:` type `)` )* `:` Sort `;`
ast_ptr hx_reader::parse_type_ctor()
{
  auto old_loc = current.loc;
  bool error = false;
  if(!expect(token_kind::Keyword, diagnostic_db::parser::type_keyword_expected))
    error = true;

  scoping_ctx.is_binding = true;
  auto type_name = parse_identifier();
  scoping_ctx.is_binding = false;
  auto type_name_id = old.data;

  if(!expect(':', diagnostic_db::parser::type_assign_expects_equal) || type_name == error_ref)
    error = true;

  scoping_ctx.disallow_recursion = type_name;
  auto tail = parse_expression();
  scoping_ctx.disallow_recursion = nullptr;

  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end) || tail == error_ref)
    error = true;

  if(error)
    return mk_error();
  type_name->annot = tail;
  return std::make_shared<assign_type>(type_name, tail);
}


ast_ptr hx_reader::parse_assign(ast_ptr to)
{
  auto var_symb = old.data;
  auto current_source_loc = old.loc;

  bool error = to == error_ref;
  if(!expect(token_kind::ColonEqual, diagnostic_db::parser::assign_expects_equal))
    error = true;
  auto arg = parse_expression();
  if(arg == error_ref)
    error = true;

  if(error)
    return mk_error();
  auto ass = std::make_shared<assign>(to, arg);
  ass->lhs->annot = arg->annot;

  return ass;
}

ast_ptr hx_reader::parse_expr_stmt()
{
  auto current_source_loc = current.loc;

  auto expr = parse_expression();

  bool error = false;
  if(expr == error_ref)
    error = true;
  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end))
  {
    source_range loc = source_range {};
    auto proj = loc.snd_proj();

    // do not want to ignore this token
    loc.column_beg = loc.column_end;
    fixits_stack.back().changes.emplace_back(proj, nlohmann::json { {"what", loc}, {"how", ";"} });

    diagnostic <<= mk_diag::fixit(current_source_loc + loc, fixits_stack.back());

    error = true;
  }
  if(error)
    return mk_error();

  return std::make_shared<expr_stmt>(expr);
}

ast_ptr hx_reader::parse_statement()
{
  ast_ptr to_ret = nullptr;
  fixits_stack.emplace_back();
  if(current.kind == token_kind::Keyword && current.data.get_hash() == symbol("type").get_hash())
    to_ret = parse_type_ctor();
  else if(current.kind == token_kind::Keyword && current.data.get_hash() == symbol("data").get_hash())
    to_ret = parse_data_ctor();
  else if(current.kind == token_kind::Hash)
    to_ret = parse_directive();
  else if(current.kind == token_kind::Identifier && next_toks[0].kind == token_kind::LParen)
    to_ret = parse_function();
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
  case hash_string("case"): return parse_case();
  }
  assert(false && "bug in lexer, we would not see a keyword token otherwise");
  return mk_error();
}

// e := `case` e `[` p1 `=>` e1 `|` ... `|` pN `=>` eN `]`
ast_ptr hx_reader::parse_case()
{
  assert(expect(token_kind::Keyword, diagnostic_db::parser::case_expects_keyword));

  bool error = false;
  auto what_to_match = parse_expression();
  if(what_to_match == error_ref)
    error = true;

  if(!expect('[', diagnostic_db::parser::case_expects_lbracket))
    error = true;

  std::vector<ast_ptr> match_arms;
  if(current.kind != token_kind::LBracket)
  {
    // we actually have match arms
    do
    {
      parsing_pattern = true;
      scoping_ctx.is_binding = true;
      auto pat = parse_expression();
      scoping_ctx.is_binding = false;
      parsing_pattern = false;

      if(!expect(token_kind::Doublearrow, diagnostic_db::parser::pattern_expects_double_arrow) || pat == error_ref)
        error = true;

      auto expr = parse_expression();

      if(expr == error_ref)
        error = true;
      match_arms.emplace_back(std::make_shared<match>(pat, expr));
    } while(accept('|'));
  }
  if(!expect(']', diagnostic_db::parser::case_expects_rbracket))
    error = true;

  if(error)
    return mk_error();
  return std::make_shared<pattern_matcher>(what_to_match, match_arms);
}

ast_ptr hx_reader::parse_with_parentheses()
{
  bool error = false;
  if(!expect('(', diagnostic_db::parser::tuple_or_unit_expr_expect_lparen))
    error = true;

  if(accept(')'))
    return error ? mk_error() : std::make_shared<unit>();

  if(current.kind == token_kind::Identifier && next_toks[0].kind == token_kind::Colon)
  {
    // type checking or lambda
    auto id = parse_identifier();

    if(id == error_ref)
      error = true;
    auto psymb = old.data;

    if(!expect(':', diagnostic_db::parser::type_ctor_param_expects_colon))
      error = true;
    auto typ = parse_expression();

    if(typ == error_ref)
      error = true;
    if(!expect(')', diagnostic_db::parser::closing_parenthesis_expected))
      return mk_error();

    id->annot = typ;

    if(accept(token_kind::Arrow))
    {
      // lambda
      scoping_ctx.binder_stack.emplace_back(psymb, id);
      auto body = parse_expression();
      scoping_ctx.binder_stack.pop_back();

      if(body == error_ref)
        error = true;

      if(error)
        return mk_error();
      return std::make_shared<lambda>(id, body);
    }
    // just an identifier with a type annotation
    if(error)
      return mk_error();
    return id;
  }
  else
  {
    // type checking or parenthesized expression
    auto expr = parse_expression();

    if(expr == error_ref)
      error = true;
    if(!expect(')', diagnostic_db::parser::closing_parenthesis_expected))
      error = true;

    if(error)
      return mk_error();
    return expr;
  }
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
      return parse_with_parentheses();
    }

    case token_kind::Identifier:
    {
      if(next_toks[0].kind == token_kind::Arrow)
        return parse_lambda();
      return parse_identifier();
    }
    case token_kind::Keyword:
    {
      return parse_keyword();
    }
    case token_kind::Backslash:
    {
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

  left->annot = right;
  
  return left;
}

// Pi    (A -> B) -> C    will be    \(_ : A -> B). C
ast_ptr hx_reader::parse_arrow_lam(ast_ptr argument)
{
  assert(expect(token_kind::Arrow, diagnostic_db::parser::lambda_expects_arrow));

  auto bdy = parse_expression();

  auto arg = std::make_shared<identifier>("_");
  arg->annot = argument;
  return std::make_shared<lambda>(arg, bdy);
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
      case token_kind::Comma:
      case token_kind::LBracket:
      case token_kind::Doublearrow:
      {
        return prefix;
      } break;

      case token_kind::Arrow:
      {
        return parse_arrow_lam(prefix);
      } break;

      case token_kind::Colon:
      {
        return parse_type_check(prefix);
      } break;

      case token_kind::ColonEqual:
      {
        return parse_assign(prefix);
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

  scoping_context ctx;
  r.scoping_ctx = ctx;

  hx_ast ast;
  while(r.current.kind != token_kind::EndOfFile)
  {
    auto stmt = r.parse_statement();

    if(stmt == r.error_ref)
    {
      continue; // please fix it, user
    }

    ast.data.push_back(stmt);
  }
  if(ast.data.empty() && diagnostic.empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(r.current.loc);

  return { ast };
}

template<>
std::vector<std::pair<hx_ast, scoping_context>> hx_reader::read_with_ctx(std::string_view module, scoping_context&& ctx)
{
  hx_reader r(module);

  r.scoping_ctx = ctx;

  hx_ast ast;
  while(r.current.kind != token_kind::EndOfFile)
  {
    auto stmt = r.parse_statement();

    if(stmt == r.error_ref)
      continue; // please fix it, user

    ast.data.push_back(stmt);
  }
  if(ast.data.empty() && diagnostic.empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(r.current.loc);

  return { { ast, r.scoping_ctx } };
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

std::pair<hx_ast, scoping_context> hx_reader::read_text(const std::string& text, scoping_context&& ctx)
{
  std::stringstream ss(text);
  hx_reader r(ss);

  r.scoping_ctx = ctx;

  hx_ast ast;
  while(r.current.kind != token_kind::EndOfFile)
  {
    auto stmt = r.parse_statement();

    if(stmt == r.error_ref)
      continue; // please fix it, user

    ast.data.push_back(stmt);
  }
  if(ast.data.empty() && diagnostic.empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(r.current.loc);

  return { ast, r.scoping_ctx };
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


