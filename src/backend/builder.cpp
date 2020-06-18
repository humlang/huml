#include <backend/builder.hpp>

#include <ostream>
#include <string>
#include <queue>

#include <iostream>

ir::builder::builder()
{
  // Bootstrap
  kind(); type(); prop();
  unit();

  auto bot = id("⊤", type());

  // "main" function's exit     () -> TOP
  world_exit = fn(unit(), bot);

  // "main" function's entry    (() -> TOP) -> TOP // TODO: add arc and argv
  world_entry = fn(world_exit, bot);
  world_entry->set_type(fn(world_exit, bot));
}

ir::Node::Ref ir::builder::kind()
{ return lookup_or_emplace(Node::mk_node<Kind>()); }

ir::Node::Ref ir::builder::type()
{ return lookup_or_emplace(Node::mk_node<Type>()); }

ir::Node::Ref ir::builder::prop()
{ return lookup_or_emplace(Node::mk_node<Prop>()); }

ir::Node::Ref ir::builder::unit()
{ return lookup_or_emplace(Node::mk_node<Unit>()); }

ir::Fn::Ref ir::builder::entry()
{ return world_entry; }

ir::Fn::Ref ir::builder::exit()
{ return world_exit; }

ir::Node::Ref ir::builder::id(symbol symb, Node::Ref type)
{
  assert(type != Node::no_ref && "Type must exist.");
  return lookup_or_emplace(Node::mk_node<Constructor>(symb, type));
}

ir::Node::Ref ir::builder::ignore() // TODO: replace nullptr with existential
{ return lookup_or_emplace(Node::mk_node<Constructor>("_", nullptr)); }

ir::Node::Ref ir::builder::param(Node::Ref type)
{ return lookup_or_emplace(Node::mk_node<Param>(type)); }

ir::Node::Ref ir::builder::lit(std::uint_fast64_t value)
{ return lookup_or_emplace(Node::mk_node<Literal>(value)); }

ir::Node::Ref ir::builder::binop(ir::BinaryKind op, Node::Ref lhs, Node::Ref rhs)
{ return lookup_or_emplace(Node::mk_node<Binary>(op, lhs, rhs)); }

ir::Node::Ref ir::builder::i(bool no_sign, Node::Ref size)
{ return lookup_or_emplace(Node::mk_node<Int>(no_sign, size)); }

ir::Fn::Ref ir::builder::fn(Node::Ref codomain, Node::Ref domain)
{
  assert(codomain != nullptr && "codomain must stay valid.");
  return static_cast<Fn::Ref>(lookup_or_emplace(Node::mk_node<Fn>(codomain, domain)));
}

ir::Node::Ref ir::builder::app(Node::Ref caller, Node::Ref arg)
{
  assert(caller != nullptr && arg != nullptr && "caller and arg must stay valid.");
  return lookup_or_emplace(Node::mk_node<App>(caller, arg));
}

ir::Node::Ref ir::builder::destruct(Node::Ref of, std::vector<std::pair<Node::Ref, Node::Ref>> match_arms)
{ return lookup_or_emplace(Node::mk_node<Case>(of, match_arms)); }

ir::Node::Ref ir::builder::lookup_or_emplace(Node::Store store)
{
  store->mach_ = this;
  store->gid_ = gid++;

  if(auto it = nodes.find(store); it != nodes.end())
      return it->get(); // <- might be different pointer
  return nodes.emplace(std::move(store)).first->get();
}

void ir::builder::print_graph(std::ostream& os)
{
  os << "digraph iea {\n";
  print_graph(os, world_entry);
  os << "}\n";
}

static std::size_t param_count = 0;
static std::size_t fn_count = 0;
static ir::NodeSet defs_printed = {};

std::ostream& ir::builder::print_graph(std::ostream& os, Node::Ref ref)
{
  switch(ref->kind())
  {
  case NodeKind::Kind: os << "Kind"; break;
  case NodeKind::Type: os << "Type"; break;
  case NodeKind::Prop: os << "Prop"; break;
  case NodeKind::Ctr:  os << static_cast<Constructor::Ref>(ref)->name.get_string(); break;
  case NodeKind::Literal: os << static_cast<Literal::Ref>(ref)->literal; break;
  case NodeKind::Int:  os << "int"; break;
  case NodeKind::Param: os << "p" << ++param_count; break;
  case NodeKind::Unit: os << "UNIT"; break;
  case NodeKind::Binary: {
      auto bin = static_cast<Binary::Ref>(ref);

      std::string op;
      switch(bin->op)
      {
      case BinaryKind::Mult:  op = "mul"; break;
      case BinaryKind::Plus:  op = "pls"; break;
      case BinaryKind::Minus: op = "min"; break;
      }
      op += std::to_string(bin->gid());

      if(!defs_printed.contains(ref))
      {
        print_graph(os, bin->lhs()) << " -> " << op << ";\n";
        print_graph(os, bin->rhs()) << " -> " << op << ";\n";

        defs_printed.insert(ref);
      }
      os << op;
    } break;
  case NodeKind::App: {
      auto ap = static_cast<App::Ref>(ref);

      std::string op = "app_" + std::to_string(ap->gid());
      if(!defs_printed.contains(ref))
      {
        print_graph(os, ap->caller()) << " -> " << op << ";\n";
        print_graph(os, ap->arg()) << " -> " << op << ";\n";

        defs_printed.insert(ref);
      }
      os << op;
    } break;
  case NodeKind::Fn: {
      auto fn = static_cast<Fn::Ref>(ref);

      std::string op = "fn_" + std::to_string(fn->gid());
      if(!defs_printed.contains(ref))
      {
        print_graph(os, fn->arg()) << " -> " << op << ";\n";
        print_graph(os, fn->bdy()) << " -> " << op << ";\n";

        defs_printed.insert(ref);
      }
      os << op;
    } break;
  case NodeKind::Case: {
      auto cs = static_cast<Case::Ref>(ref);

      std::string op = "case_" + std::to_string(cs->gid());
      if(!defs_printed.contains(ref))
      {
        auto vals = cs->match_arms();
        print_graph(os, cs->of()) << " -> " << op << ";\n";
        for(auto& v : vals)
        {
          print_graph(os, v.first)  << " -> " << op << ";\n";
          print_graph(os, v.second) << " -> " << op << ";\n";
        }

        defs_printed.insert(ref);
      }
      os << op;
    } break;
  }

  return os;
}

