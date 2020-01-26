#include <diagnostic.hpp>
#include <diagnostic_db.hpp>
#include <memory>
#include <stream_lookup.hpp>

#include <tmp/cxset.hpp>
#include <tmp/cxmap.hpp>

#include <reader.hpp>
#include <token.hpp>
#include <ast.hpp>

#include <string_view>
#include <cassert>
#include <istream>
#include <queue>
#include <vector>

using namespace std::literals::string_view_literals;

constexpr auto keyword_set = make_set<std::string_view>({

    // modules
    "import"sv,
    "for"sv
});

constexpr auto operator_symbols_map = make_map<std::string_view, token_kind>({

        { "."sv, token_kind::Point },
        {":"sv, token_kind::Colon},
        {";"sv, token_kind::Semi},
        {"+"sv,  token_kind::Plus},
        {"-"sv,  token_kind::Minus},
        {"*"sv,  token_kind::Asterisk},
        {"="sv,  token_kind::Equal}
});
// ordered in increasing preference
constexpr auto token_precedence_map = make_map<token_kind, int>( {
        {token_kind::Plus, 1},
        {token_kind::Minus, 1},
        {token_kind::Asterisk, 2},
        {token_kind::LiteralNumber, 6},

        });


reader::reader(std::string_view module)
  : module(module), linebuf(), is(stream_lookup[module]), col(0), row(1)
{
  for(std::size_t i = 0; i < next_toks.size(); ++i)
    consume();
}

reader::~reader()
{ stream_lookup.drop(module); }

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


///// Parsing

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
        // TODO I dont know how unperfomant this is and if there is a better way to convert a char to a string view
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
      // read until there is no number anymore. Disallow 000840
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
        if(std::iscntrl(ch) || std::isspace(ch) || operator_symbols_map.contains(ch_s.c_str()) || keyword_set.contains(name.c_str()))
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
        diagnostic <<= (diagnostic_db::parser::leading_zeros(name) + source_range(module, beg_col + 1, beg_row, col + 1, row + 1))
          | source_context(0);
        goto restart_get; // <- SO WHAT
      }
      if(emit_not_a_number_error)
      {
        diagnostic <<= (diagnostic_db::parser::no_number(name) + source_range(module, beg_col + 1, beg_row, col + 1, row + 1))
                      | source_context(0);
        goto restart_get;
      }
      kind = token_kind::LiteralNumber;
      data = symbol(name);
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
  case ':': // used for assign in :=
  {
    assert(col < linebuf.size());
      ch = linebuf[col++];
      if (ch == '=')
      {
        kind = token_kind::Assign;
        data = ":=";
      }
      else
      {
        kind = token_kind::Colon;
        data = ":";
        col--; // revert look ahead try
      }
  } break;
  case EOF:
      kind = token_kind::EndOfFile;
    break;
  }
  return token(kind, data, {module, beg_col + 1, beg_row, col + 1, row + 1});
}

void reader::consume()
{
  old = current;
  current = next_toks[0];

  for(std::size_t i = 0, j = 1; j < next_toks.size(); ++i, ++j)
    std::swap(next_toks[i], next_toks[j]);

  next_toks.back() = get<token>();
}

template<typename F>
void reader::expect(token_kind kind, F&& f)
{
  if(current.kind != kind)
    diagnostic <<= (f + next_toks[0].loc) | source_context(0);
  consume();
}

template<typename F>
void reader::expect(std::int8_t c, F&& f)
{
  expect(static_cast<token_kind>(c), std::forward<F>(f));
}

bool reader::accept(token_kind kind)
{
  if(current.kind != kind)
    return false;

  consume();
  return true;
}

bool reader::accept(std::int8_t c)
{
  return accept(static_cast<token_kind>(c));
}

rec_wrap_t<literal> reader::parse_literal()
{
  consume();
  return std::make_unique<literal>(ast_tags::literal, old);
}

rec_wrap_t<identifier> reader::parse_identifier()
{
  consume();
  return std::make_unique<identifier>(ast_tags::identifier, old);
}

rec_wrap_t<block> reader::parse_block()
{
  auto tmp = current;
  expect('{', diagnostic_db::parser::block_expects_lbrace(current.data.get_string()));

  std::vector<stmt_type> v;
  while(!accept('}'))
    v.emplace_back(parse_statement());

  return std::make_unique<block>(ast_tags::block, tmp, std::move(v));
}

stmt_type reader::parse_keyword()
{
  assert(keyword_set.contains(current.data.get_string()));

  consume();
  if("for" == old.data.get_string())
  {
    auto very_old = old;
    auto l = parse_literal();
    auto b = parse_block();
    return std::make_unique<loop>(ast_tags::loop, very_old, std::move(l), std::move(b));
  }

  return std::make_unique<error>(ast_tags::error, old);
}

stmt_type reader::parse_assign()
{
  // we know we have an identifier here
  auto id = parse_identifier();
  // check if next sign is an = for
  expect(4, diagnostic_db::parser::assign_expects_colon_equal(current.data.get_string())); // TODO this has to be changed expect consumes
  token assign_tok = old;    // get the assign
  // the next part is an expression
  exp_type right = parse_expression(0);
  return std::make_unique<assign>(ast_tags::assign, std::move(id), assign_tok, std::move(right));
}

stmt_type reader::parse_statement()
{
  switch(current.kind)
  {
  default:
  {
    diagnostic <<= (diagnostic_db::parser::unknown_token(current.data.get_string()) + current.loc)
                  | source_context(0);
    consume();
    return std::make_unique<error>(ast_tags::error, old);
  } break;

  case token_kind::Keyword:
  {
    return std::move(parse_keyword());
  }
  case token_kind::Identifier:
  {
    return std::move(parse_assign());
  }
  }
}

exp_type reader::parse_prefix() // operator
{
  switch(current.kind)
  {
    default:
    {
      diagnostic <<= (diagnostic_db::parser::unknown_token(current.data.get_string()) + current.loc) // expected prefix expression
                     | source_context(0);
      consume();
      return std::make_unique<error>(ast_tags::error, old); // return error when we did not find a fitting token
    }

    case token_kind::LiteralNumber:
    {
      return std::move(parse_literal());
    }
  }
}


exp_type reader::parse_binary(exp_type left)
{
  token op = current; // safe operator
  consume(); // get to next token
  int precedence = token_precedence_map[op.kind]; // this is for *
  exp_type right = parse_expression(precedence);
  return std::make_unique<binary_exp>(ast_tags::binary_exp, std::move(left), op, std::move(right)); // I hope this calls the constructor
}

exp_type reader::parse_expression(int precedence)
{

  exp_type prefix = parse_prefix(); // this consumes one right now by creating a literal e.g 3 x was called prefix

  if(std::holds_alternative<rec_wrap_t<error>>(prefix)) { // if we have an error in left
    // error state that should not happen
  }

  // Infix part because of move semantics we cant write it inside a function since we would need to move prefix that we wpuld maybe return
  exp_type infix;  // e.g + - = etc since i move my prefix here i lose it if i want to return it in my if condition
  // TODO we have to be careful on how we set these precedences in the functions. sometimes map is enough but sometimes we need the context e-g unary - or binary -?
  // TODO For printing I want to print the actual AST values in the nodes
  // TODO should parse_read() be under parse_keyword()? or own function?
  // Will we add funcitons to the ast attributes and do we need to add them to all or is something else planned?

  // we cant make this into function parse_infix since we need to move prefix then..
  while(precedence < getPrecedence()) { // that way we would handle left associativity think about 2 * 3 + 5 => would be 2* (3 + 5) which is wrong
    switch(current.kind)
    {
      default:
      { // TODO this should never happen anymore
        return prefix; // Important if there is no infix we return the prefix
        //infix = std::make_unique<error>(ast_tags::error, old); // return error when we did not find a fitting token
      }

      case token_kind::Plus: case token_kind::Minus: case token_kind::Asterisk:
      {
        prefix = parse_binary(std::move(prefix));
      }
    }
  }


  return prefix;
}


std::vector<ast_type> reader::read(std::string_view module)
{
  reader r(module);

  std::vector<ast_type> ast;
  while(r.current.kind != token_kind::EndOfFile)
  {
    r.consume();
    /*
    switch(r.current.kind)
    {
    default:
    {
      diagnostic <<= (diagnostic_db::parser::unknown_token(r.current.data.get_string()) + r.current.loc)
                    | source_context(0);
    } break;

    case token_kind::EndOfFile:
    break;

    }*/
    //auto tmp2 = exp_type_to_ast_type(r.parse_expression(0));
    auto tmp = stmt_type_to_ast_type(r.parse_statement());
    ast.emplace_back(std::move(tmp));
  }
  if(ast.empty())
  {
    diagnostic <<= (-diagnostic_db::parser::empty_module);
  }
  return ast;
}

// returns precedence of the lookahead token! uses a map
int reader::getPrecedence() {
  token_kind prec = current.kind; // I think it should be current
  if (prec == token_kind::Undef || prec == token_kind::EndOfFile || prec == token_kind::Semi)
    return 0;
  return token_precedence_map[prec];
}

