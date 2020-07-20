#include <reader.hpp>
#include <diagnostic.hpp>
#include <diagnostic_db.hpp>

#include <tsl/robin_map.h>

#include <sstream>

const ast_ptr huml_reader::error_ref = nullptr;

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

void huml_reader::consume()
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

ast_ptr huml_reader::mk_error()
{ return error_ref; }

ast_ptr huml_reader::parse_directive()
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
ast_ptr huml_reader::parse_identifier()
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

  return std::make_shared<identifier>(old.data);
}

// e := e1 e2
ast_ptr huml_reader::parse_app(ast_ptr lhs)
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
ast_ptr huml_reader::parse_lambda()
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
  auto expr = parse_expression();

  if(expr == error_ref)
    error = true;

  if(error)
    return mk_error();

  lam_tok.loc += old.loc;
// TODO:  global_scope.dbg_data[to_ret].loc = lam_tok;
  return std::make_shared<lambda>(param, expr);
}

// s := id (a : type)...(z : type) := e ;
ast_ptr huml_reader::parse_function(bool no_body)
{
  auto fn_id = parse_identifier();
  auto fn_name = old.data;

  bool error = fn_id == error_ref;

  std::vector<ast_ptr> params;
  if(!expect('(', diagnostic_db::parser::type_expects_lparen))
    error = true;
  do {
    std::vector<std::pair<ast_ptr, symbol>> ids_of_same_type;
    ids_of_same_type.reserve(32);

    // each parameter corresponds to a new lambda!
    {
    parsing_pattern = true;
    auto id = parse_identifier();
    parsing_pattern = false;
    ids_of_same_type.emplace_back(id, old.data);
    while(current.kind == token_kind::Identifier)
    {
      // each parameter corresponds to a new lambda!
      parsing_pattern = true;
      id = parse_identifier();
      parsing_pattern = false;
      ids_of_same_type.emplace_back(id, old.data);
    }
    }
    if(!expect(':', diagnostic_db::parser::lambda_param_type_decl_expects_colon))
      error = true;
    auto typ = parse_expression();

    for(auto& p : ids_of_same_type)
    {
      p.first->annot = typ;
      params.push_back(p.first);
    }
    if(!expect(')', diagnostic_db::parser::closing_parenthesis_expected))
      error = true;
  } while(accept('('));

  // check if the return type is annotated
  ast_ptr return_type = nullptr;
  if(!no_body && accept(token_kind::Arrow))
  {
    return_type = parse_expression();

    if(return_type == error_ref)
      error = true;
  }
  else if(no_body)
  {
    if(!expect(token_kind::Arrow, diagnostic_db::parser::lambda_expects_arrow))
      error = true;

    return_type = parse_expression();
    if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end) || return_type == error_ref)
      error = true;

    if(error)
      return mk_error();
    lambda::ptr lam = std::make_shared<lambda>(params.back(), return_type);

    for(auto it = params.rbegin() + 1; it != params.rend(); ++it)
      lam = std::make_shared<lambda>(*it, lam);
    return std::make_shared<assign>(fn_id, lam, nullptr);
  }
  if(!expect(token_kind::ColonEqual, diagnostic_db::parser::function_expects_colon_eq))
    error = true;

  auto expr = parse_expression();
  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end) || expr == error_ref)
    error = true;
  if(error)
    return mk_error();

  expr->annot = return_type;
  lambda::ptr lam = std::make_shared<lambda>(params.back(), expr);
  for(auto it = params.rbegin() + 1; it != params.rend(); ++it)
    lam = std::make_shared<lambda>(*it, lam);
  return std::make_shared<assign>(fn_id, lam, nullptr);
}

// e := Kind
ast_ptr huml_reader::parse_Kind()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Kind))
    return mk_error();
  return std::make_shared<kind>();
}

// e := Type
ast_ptr huml_reader::parse_Type()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Type))
    return mk_error();
  return std::make_shared<type>();
}

// e := Prop
ast_ptr huml_reader::parse_Prop()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Prop))
    return mk_error();
  return std::make_shared<prop>();
}

// e := Trait
ast_ptr huml_reader::parse_Trait()
{
  if(!expect(token_kind::Keyword, diagnostic_db::parser::expected_keyword_Trait))
    return mk_error();
  return std::make_shared<trait_type>();
}

// s := `data` name ( `(` id `:` type `)` )* `:` type `;`
ast_ptr huml_reader::parse_data_ctor()
{
  auto old_loc = current.loc;
  bool error = false;
  if(!expect(token_kind::Keyword, diagnostic_db::parser::type_keyword_expected))
    error = false;

  auto type_name = parse_identifier();
  auto type_name_id = old.data;

  if(!expect(':', diagnostic_db::parser::type_assign_expects_colon) || type_name == error_ref)
    error = true;

  auto tail = parse_expression();

  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end) || tail == error_ref)
    error = true;

  if(error)
    return mk_error();

  type_name->annot = tail;
  return std::make_shared<assign_data>(type_name, tail);
}

// s := `type` name ( `(` id `:` type `)` )* `:` Sort `;`
ast_ptr huml_reader::parse_type_ctor()
{
  auto old_loc = current.loc;
  bool error = false;
  if(!expect(token_kind::Keyword, diagnostic_db::parser::type_keyword_expected))
    error = true;

  auto type_name = parse_identifier();
  auto type_name_id = old.data;

  if(!expect(':', diagnostic_db::parser::type_assign_expects_colon) || type_name == error_ref)
    error = true;

  auto tail = parse_expression();

  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end) || tail == error_ref)
    error = true;

  if(error)
    return mk_error();
  type_name->annot = tail;
  return std::make_shared<assign_type>(type_name, tail);
}

// e := let x := y; e
ast_ptr huml_reader::parse_assign()
{
  auto type_name = parse_identifier();

  auto var_symb = old.data;
  auto current_source_loc = old.loc;

  bool error = type_name == error_ref;
  if(!expect(token_kind::ColonEqual, diagnostic_db::parser::assign_expects_equal))
    error = true;
  auto arg = parse_expression();
  if(arg == error_ref)
    error = true;

  if(!expect(token_kind::Semi, diagnostic_db::parser::statement_expects_semicolon_at_end) || error)
    return mk_error();

  auto ass = std::make_shared<assign>(type_name, arg, parse_expression());
  ass->identifier->annot = arg->annot;

  return ass;
}

ast_ptr huml_reader::parse_expr_stmt()
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

// s := trait id param-list { function-decl+; };
ast_ptr huml_reader::parse_trait()
{
  assert(accept(token_kind::Keyword) && "expected \"trait\" keyword");
  auto trait_name = parse_identifier();
  auto trait_name_id = old.data;

  bool error = trait_name == error_ref;
  if(error)
    return mk_error();

  std::vector<ast_ptr> params;
  if(!expect('(', diagnostic_db::parser::type_expects_lparen))
    error = true;
  do {
    std::vector<std::pair<ast_ptr, symbol>> ids_of_same_type;
    ids_of_same_type.reserve(32);

    // each parameter corresponds to a new lambda!
    {
    parsing_pattern = true;
    auto id = parse_identifier();
    parsing_pattern = false;
    ids_of_same_type.emplace_back(id, old.data);
    while(current.kind == token_kind::Identifier)
    {
      // each parameter corresponds to a new lambda!
      parsing_pattern = true;
      id = parse_identifier();
      parsing_pattern = false;
      ids_of_same_type.emplace_back(id, old.data);
    }
    }
    if(!expect(':', diagnostic_db::parser::lambda_param_type_decl_expects_colon))
      error = true;
    auto typ = parse_expression();

    for(auto& p : ids_of_same_type)
    {
      p.first->annot = typ;
      params.push_back(p.first);
    }
    if(!expect(')', diagnostic_db::parser::closing_parenthesis_expected))
      error = true;
  } while(accept('('));

  if(!expect('{', diagnostic_db::parser::trait_decl_expects_lbrace))
    error = true;

  std::vector<ast_ptr> fns;
  do
  {
    auto fn = parse_function(true);
    if(fn == error_ref)
      error = true;
    else
      fns.emplace_back(fn);
  } while(accept(';'));

  if(!expect('}', diagnostic_db::parser::trait_decl_expects_rbrace))
    error = true;

  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end))
    error = true;

  if(error)
    return mk_error();
  auto ret = std::make_shared<trait>(trait_name, params, fns);

  auto it = params.rbegin();
  auto lam = std::make_shared<lambda>(*it, std::make_shared<trait_type>());
  for(it = it + 1; it != params.rend(); ++it)
    lam = std::make_shared<lambda>(*it, lam);
  return ret;
}

// s := implement expr { function+; };
ast_ptr huml_reader::parse_implement()
{
  assert(accept(token_kind::Keyword) && "expected \"implement\" keyword");
  auto trait = parse_expression();

  bool error = trait == error_ref;
  if(error)
    return mk_error();

  if(!expect('{', diagnostic_db::parser::trait_decl_expects_lbrace))
    error = true;

  std::vector<ast_ptr> fns;
  do
  {
    auto fn = parse_function();
    fns.emplace_back(fn);
  } while(accept(';'));

  if(!expect('}', diagnostic_db::parser::trait_decl_expects_rbrace))
    error = true;
  if(!expect(';', diagnostic_db::parser::statement_expects_semicolon_at_end))
    error = true;
  if(error)
    return mk_error();

  return std::make_shared<implement>(trait, fns);
}

ast_ptr huml_reader::parse_statement()
{
  ast_ptr to_ret = nullptr;
  fixits_stack.emplace_back();
  if(current.kind == token_kind::Keyword)
  {
    switch(current.data.get_hash())
    {
    case hash_string("type"): to_ret = parse_type_ctor(); break;
    case hash_string("data"): to_ret = parse_data_ctor(); break;
    case hash_string("trait"): to_ret = parse_trait(); break;
    case hash_string("implement"): to_ret = parse_implement(); break;
    }
  }
  else if(current.kind == token_kind::Hash)
    to_ret = parse_directive();
  else if(current.kind == token_kind::Identifier && next_toks[0].kind == token_kind::LParen)
    to_ret = parse_function();
  else
    to_ret = parse_expr_stmt();
  fixits_stack.pop_back();
  return to_ret;
}

ast_ptr huml_reader::parse_keyword()
{
  switch(current.data.get_hash())
  {
  case hash_string("Type"):  return parse_Type();
  case hash_string("Kind"):  return parse_Kind();
  case hash_string("Prop"):  return parse_Prop();
  case hash_string("Trait"): return parse_Trait();
  case hash_string("case"):  return parse_case();
  case hash_string("let"):   return parse_assign();
  }
  assert(false && "bug in lexer, we would not see a keyword token otherwise");
  return mk_error();
}

// e := `case` e `[` p1 `=>` e1 `|` ... `|` pN `=>` eN `]`
ast_ptr huml_reader::parse_case()
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
      auto pat = parse_expression();
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

ast_ptr huml_reader::parse_with_parentheses()
{
  bool error = false;
  if(!expect('(', diagnostic_db::parser::tuple_or_unit_expr_expect_lparen))
    error = true;

  if(accept(')'))
    return error ? mk_error() : std::make_shared<unit>();

  if(current.kind == token_kind::Identifier && next_toks[0].kind == token_kind::Colon)
  {
    std::vector<std::pair<ast_ptr, symbol>> ids_of_same_type;
    ids_of_same_type.reserve(32);

    // type checking or lambda
    {
    parsing_pattern = true;
    auto id = parse_identifier();
    parsing_pattern = false;
    ids_of_same_type.emplace_back(id, old.data);
    while(current.kind == token_kind::Identifier)
    {
      id = parse_identifier();
      ids_of_same_type.emplace_back(id, old.data);
    }
    }
    if(!expect(':', diagnostic_db::parser::type_ctor_param_expects_colon))
      error = true;
    auto typ = parse_expression();

    if(typ == error_ref)
      error = true;
    if(!expect(')', diagnostic_db::parser::closing_parenthesis_expected))
      return mk_error();

    for(auto& p : ids_of_same_type)
      p.first->annot = typ;

    if(accept(token_kind::Arrow))
    {
      auto body = parse_expression();
      if(body == error_ref)
        error = true;

      if(error)
        return mk_error();
      ast_ptr cur = std::make_shared<lambda>(ids_of_same_type.back().first, body);
      for(auto it = ids_of_same_type.rbegin() + 1; it != ids_of_same_type.rend(); ++it)
        cur = std::make_shared<lambda>(it->first, cur);
      return cur;
    }
    // just an identifier with a type annotation
    if(error || ids_of_same_type.size() > 1) // TODO: emit useful diagnostic in the latter case
      return mk_error();
    return ids_of_same_type.front().first;
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
ast_ptr huml_reader::parse_prefix()
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

    case token_kind::LiteralNumber:
    {
      consume();

      return std::make_shared<number>(old.data);
    }

    case token_kind::Asterisk:
    {
      consume();
      // Parse pointer thing, i.e. **i8
      auto exp = parse_expression();

      return std::make_shared<pointer>(exp);
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
ast_ptr huml_reader::parse_type_check(ast_ptr left)
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
ast_ptr huml_reader::parse_arrow_lam(ast_ptr argument)
{
  assert(expect(token_kind::Arrow, diagnostic_db::parser::lambda_expects_arrow));

  auto bdy = parse_expression();
  auto arg = std::make_shared<identifier>("_");
  arg->annot = argument;
  return std::make_shared<lambda>(arg, bdy);
}

// e
ast_ptr huml_reader::parse_expression(int precedence)
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
      case token_kind::LBrace:
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
std::vector<huml_ast> huml_reader::read(std::string_view module)
{
  huml_reader r(module);

  huml_ast ast;
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
std::vector<token> huml_reader::read(std::string_view module)
{
  huml_reader r(module);

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

huml_ast huml_reader::read_text(const std::string& text)
{
  std::stringstream ss(text);
  huml_reader r(ss);

  huml_ast ast;
  while(r.current.kind != token_kind::EndOfFile)
  {
    auto stmt = r.parse_statement();

    if(stmt == r.error_ref)
      continue; // please fix it, user

    ast.data.push_back(stmt);
  }
  if(ast.data.empty() && diagnostic.empty()) // only emit "empty module" if there hasn't been any diagnostic anyway
    diagnostic <<= diagnostic_db::parser::empty_module(r.current.loc);

  return { ast };
}

int huml_reader::precedence() {
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


