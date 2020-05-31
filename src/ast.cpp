#include <ast.hpp>
#include <type_checking.hpp>

#include <iterator>

#include <iostream>

hx_ast::hx_ast()
{
  // Kind
  types.push_back(ASTTags::Kind.make_node(*this, ASTData { "Kind" }, ASTDebugData {}));

  // Type
  types.push_back(ASTTags::Type.make_node(*this, ASTData { "Type" }, ASTDebugData {}));

  // Prop
  types.push_back(ASTTags::Prop.make_node(*this, ASTData { "Prop" }, ASTDebugData {}));

  // Unit    is inhabited by  \\(A : Type). \\(a : A). a
  types.push_back(ASTTags::lambda.make_node(*this, ASTData { "1", 2 }, ASTDebugData {}));

  auto A = ASTTags::identifier.make_node(*this, ASTData { "A", 0, Type_sort_idx }, ASTDebugData {});
  types.push_back(A);

  auto inner = ASTTags::lambda.make_node(*this, ASTData { "", 2, }, ASTDebugData {});

  auto underscore = ASTTags::identifier.make_node(*this, ASTData { "a", 0, A }, ASTDebugData {});  
  ASTTags::identifier.make_node(*this, ASTData { "a", 0, A }, ASTDebugData {  });
}

std::uint_fast32_t hx_ast::add(ASTNodeKind kind, ASTData&& dat, ASTDebugData&& debug)
{
  kinds.insert(kinds.end(), kind);
  data.emplace(data.end(), std::move(dat));
  dbg_data.emplace(dbg_data.end(), std::move(debug));

  return kinds.size() - 1;
}

template<>
std::uint_fast32_t hx_ast::add<std::size_t>(std::size_t insert_at, ASTNodeKind kind, ASTData&& dat, ASTDebugData&& debug)
{
  kinds.insert(kinds.begin() + insert_at, kind);
  data.emplace(data.begin() + insert_at, std::move(dat));
  dbg_data.emplace(dbg_data.begin() + insert_at, std::move(debug));

  return insert_at;
}

// [A / alpha]C                                         C                       A                      alpha
std::uint_fast32_t hx_ast::subst(std::uint_fast32_t in, std::uint_fast32_t what, std::uint_fast32_t with)
{
  if(in == what)
    return with;
  for(std::size_t i = 0; i < data[in].argc; ++i)
    in = subst(in + 1, what, with);

  return in;
}

bool hx_ast::type_checks()
{
#if 0
  hx_ast_type_checking typch(*this);
  typing_context ctx;
  ctx.reserve(1024);

  bool success = true;
  for(std::size_t stmt = 0; stmt < kinds.size(); ++stmt)
  {
    auto t = typch.synthesize(ctx, stmt, 0);

    if(t == static_cast<std::uint_fast32_t>(-1))
    {
      print_node(std::cout, stmt); std::cout << " # Does not typecheck.\n";
    }
    else
    {
      print_node(std::cout, stmt); std::cout << " # Has type "; print_node(std::cout, t);
      std::cout << "\n";
    }
    success = success && (t != static_cast<std::uint_fast32_t>(-1));
  }
  return success;
#endif
  return false;
}

void hx_ast::print(std::ostream& os)
{
  assert(!kinds.empty() && "AST cannot be zero, there is never an empty module!");

  for(auto& r : roots)
  {
    print_node(os, r);
    os << "\n";
  }
}

std::uint_fast32_t hx_ast::print_node(std::ostream& os, std::uint_fast32_t node)
{
  auto cpy = node;
  auto& argc = data[node].argc;
  bool print_type = data[node].type_annot != ASTData::no_type;

  print_type = print_type && kinds[node] != ASTNodeKind::assign_type && kinds[node] != ASTNodeKind::assign_data;

  if(print_type)
    os << "(";
  switch(kinds[node])
  {
    case ASTNodeKind::undef:      os << "undef"; break;
    case ASTNodeKind::unit:       os << "()"; break;
    case ASTNodeKind::Kind:       os << "Kind"; break;
    case ASTNodeKind::Type:       os << "Type"; break;
    case ASTNodeKind::Prop:       os << "Prop"; break;
    case ASTNodeKind::app:        { os << "("; node = print_node(os, node + 1);
                                   os << ") ("; node = print_node(os, node + 1);
                                   os << ")"; } break;
    case ASTNodeKind::lambda:     {
        os << "\\";
        node = print_node(os, node + 1);
        os << ". ";
        node = print_node(os, node + 1);
      } break;
    case ASTNodeKind::match:      { node = print_node(os, node + 1); os << " => ";
                                   node = print_node(os, node + 1); } break;
    case ASTNodeKind::pattern:    node = print_node(os, node + 1); break;
    case ASTNodeKind::pattern_matcher: {
      os << "case (";
      node = print_node(os, node + 1);
      os << ") [";
      for(std::uint_fast32_t i = 1; i < data[cpy].argc; ++i)
      {
        node = print_node(os, node + 1);
        if(i + 1 < data[cpy].argc)
          os << " | ";
      }
      os << "]";
    } break;
    case ASTNodeKind::param:      node = print_node(os, node + 1); break;
    case ASTNodeKind::identifier: os << data[node].name.get_string(); break;
    case ASTNodeKind::assign:     { node = print_node(os, node + 1); os << " = ";
                                   node = print_node(os, node + 1); os << ";"; } break;
    case ASTNodeKind::assign_type: {
      os << "type ";
      node = print_node(os, node + 1);
      os << " : ";
      print_node(os, cpy + data[cpy].type_annot);
      os << ";";
    } break;
    case ASTNodeKind::assign_data: {
      os << "data ";
      node = print_node(os, node + 1);
      os << " : ";
      print_node(os, cpy + data[cpy].type_annot);
      os << ";";
    } break;
    case ASTNodeKind::expr_stmt:  { node = print_node(os, node + 1); os << ";"; } break;
  }
  if(print_type)
  {
    os << " : ";
    node = print_node(os, cpy + data[cpy].type_annot);
    os << ")";
  }
  return node;
}

