#include <ir.hpp>
#include <types.hpp>
#include <type_checking.hpp>

#include <iterator>

#include <iostream>

hx_ir::hx_ir()
{  }

std::uint_fast32_t hx_ir::add(IRNodeKind kind, IRData&& dat, IRDebugData&& debug)
{
  kinds.insert(kinds.end(), kind);
  data.emplace(data.end(), std::move(dat));
  dbg_data.emplace(dbg_data.end(), std::move(debug));

  return kinds.size() - 1;
}

bool hx_ir::type_checks()
{
  hx_ir_type_checking typch(typtab, *this);
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
  }
}

std::uint_fast32_t hx_ir::print_node(std::ostream& os, std::uint_fast32_t node)
{
  auto& args = data[node].args;
  switch(kinds[node])
  {
    case IRNodeKind::undef:      os << "undef"; break;
    case IRNodeKind::unit:       os << "()"; break;
    case IRNodeKind::Kind:       os << "Kind"; break;
    case IRNodeKind::Type:       os << "Type"; break;
    case IRNodeKind::Prop:       os << "Prop"; break;
    case IRNodeKind::type_check: { auto cpy = node; os << "(("; node = print_node(os, args.front());
                                   os << ") : "; print_node(os, data[cpy].type_annot); os << ")"; } break;
    case IRNodeKind::app:        { os << "("; node = print_node(os, args.front());
                                   os << ") ("; node = print_node(os, args.back());
                                   os << ")"; } break;
    case IRNodeKind::lambda:     {
        auto cpy = node;
        if(data[cpy].type_annot != IRData::no_type)
        {
          os << "\\(";
          node = print_node(os, args.front());
          os << " : ";
          print_node(os, data[cpy].type_annot);
          os << "). ";
        }
        else
        {
          os << "\\";
          node = print_node(os, args.front());
          os << ". ";
        }
        node = print_node(os, args.back());
      } break;
    case IRNodeKind::match:      { node = print_node(os, args.front()); os << " => ";
                                   node = print_node(os, args.back()); } break;
    case IRNodeKind::pattern:    node = print_node(os, args.front()); break;
    case IRNodeKind::pattern_matcher: {
      os << "case (";
      node = print_node(os, args.front());
      os << ") [";
      for(std::uint_fast32_t i = 1; i < args.size(); ++i)
      {
        node = print_node(os, args[i]);
        if(i + 1 < args.size())
          os << " | ";
      }
      os << "]";
    } break;
    case IRNodeKind::param:      node = print_node(os, args.front()); break;
    case IRNodeKind::identifier: os << data[node].name.get_string(); break;
    case IRNodeKind::assign:     { node = print_node(os, args.front()); os << " = ";
                                   node = print_node(os, args.front()); os << ";"; } break;
    case IRNodeKind::assign_type: {
      os << "type ";
      node = print_node(os, data[node].type_annot);
      os << ";";
    } break;
    case IRNodeKind::assign_data: {
      os << "data " << data[node].name << " : ";
      node = print_node(os, data[node].type_annot);
      os << ";";
    } break;
    case IRNodeKind::expr_stmt:  { node = print_node(os, args.front()); os << ";"; } break;
  }
  return node;
}

