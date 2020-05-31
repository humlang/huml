#include <ir.hpp>
#include <type_checking.hpp>

#include <iterator>

#include <iostream>

hx_ir::hx_ir()
{
  // Kind
  types.push_back(IRTags::Kind.make_node(*this, IRData { "Kind" }, IRDebugData {}));

  // Type
  types.push_back(IRTags::Type.make_node(*this, IRData { "Type" }, IRDebugData {}));

  // Prop
  types.push_back(IRTags::Prop.make_node(*this, IRData { "Prop" }, IRDebugData {}));

  // Unit    is inhabited by  \\(A : Type). \\(a : A). a
  types.push_back(IRTags::lambda.make_node(*this, IRData { "1", 2 }, IRDebugData {}));

  auto A = IRTags::identifier.make_node(*this, IRData { "A", 0, Type_sort_idx }, IRDebugData {});
  types.push_back(A);

  auto inner = IRTags::lambda.make_node(*this, IRData { "", 2, }, IRDebugData {});

  auto underscore = IRTags::identifier.make_node(*this, IRData { "a", 0, A }, IRDebugData {});  
  IRTags::identifier.make_node(*this, IRData { "a", 0, A }, IRDebugData {  });
}

std::uint_fast32_t hx_ir::add(IRNodeKind kind, IRData&& dat, IRDebugData&& debug)
{
  kinds.insert(kinds.end(), kind);
  data.emplace(data.end(), std::move(dat));
  dbg_data.emplace(dbg_data.end(), std::move(debug));

  return kinds.size() - 1;
}

template<>
std::uint_fast32_t hx_ir::add<std::size_t>(std::size_t insert_at, IRNodeKind kind, IRData&& dat, IRDebugData&& debug)
{
  kinds.insert(kinds.begin() + insert_at, kind);
  data.emplace(data.begin() + insert_at, std::move(dat));
  dbg_data.emplace(dbg_data.begin() + insert_at, std::move(debug));

  return insert_at;
}

// [A / alpha]C                                         C                       A                      alpha
std::uint_fast32_t hx_ir::subst(std::uint_fast32_t in, std::uint_fast32_t what, std::uint_fast32_t with)
{
  if(in == what)
    return with;
  for(std::size_t i = 0; i < data[in].argc; ++i)
    in = subst(in + 1, what, with);

  return in;
}

bool hx_ir::type_checks()
{
  hx_ir_type_checking typch(*this);
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
}

void hx_ir::print(std::ostream& os)
{
  assert(!kinds.empty() && "IR cannot be zero, there is never an empty module!");

  for(auto& r : roots)
  {
    print_node(os, r);
    os << "\n";
  }
}

std::uint_fast32_t hx_ir::print_node(std::ostream& os, std::uint_fast32_t node)
{
  auto cpy = node;
  auto& argc = data[node].argc;
  switch(kinds[node])
  {
    case IRNodeKind::undef:      os << "undef"; break;
    case IRNodeKind::unit:       os << "()"; break;
    case IRNodeKind::Kind:       os << "Kind"; break;
    case IRNodeKind::Type:       os << "Type"; break;
    case IRNodeKind::Prop:       os << "Prop"; break;
    case IRNodeKind::type_check: { os << "(("; node = print_node(os, node + 1);
                                   os << ") : "; print_node(os, cpy + data[cpy].type_annot); os << ")"; } break;
    case IRNodeKind::app:        { os << "("; node = print_node(os, node + 1);
                                   os << ") ("; node = print_node(os, node + 1);
                                   os << ")"; } break;
    case IRNodeKind::lambda:     {
        auto cpy = node;
        if(data[cpy].type_annot != IRData::no_type)
        {
          os << "\\(";
          node = print_node(os, node + 1);
          os << " : ";
          print_node(os, cpy + data[cpy].type_annot);
          os << "). ";
        }
        else
        {
          os << "\\";
          node = print_node(os, node + 1);
          os << ". ";
        }
        node = print_node(os, node + 1);
      } break;
    case IRNodeKind::match:      { node = print_node(os, node + 1); os << " => ";
                                   node = print_node(os, node + 1); } break;
    case IRNodeKind::pattern:    node = print_node(os, node + 1); break;
    case IRNodeKind::pattern_matcher: {
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
    case IRNodeKind::param:      node = print_node(os, node + 1); break;
    case IRNodeKind::identifier: os << data[node].name.get_string(); break;
    case IRNodeKind::assign:     { node = print_node(os, node + 1); os << " = ";
                                   node = print_node(os, node + 1); os << ";"; } break;
    case IRNodeKind::assign_type: {
      os << "type " << data[node].name << " : ";
      print_node(os, node + data[node].type_annot);
      os << ";";
    } break;
    case IRNodeKind::assign_data: {
      auto cpy = node;
      os << "data "; node = print_node(os, node + 1); os << " : ";
      print_node(os, cpy + data[cpy].type_annot);
      os << ";";
    } break;
    case IRNodeKind::expr_stmt:  { node = print_node(os, node + 1); os << ";"; } break;
  }
  return node;
}

