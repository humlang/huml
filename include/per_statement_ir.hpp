#pragma once

#include <token.hpp>
#include <types.hpp>
#include <cstdint>
#include <vector>

enum class IRNodeKind : std::int_fast8_t
{
  undef = 0,
  literal,
  unit,
  top,
  bot,
  app,
  access,
  tuple,
  lambda,
  param,
  pattern,
  match,
  pattern_matcher,
  identifier,
  block,
  assign,
  assign_type,
  expr_stmt,
  binary_exp
};

struct IRData
{
  symbol symb;
  std::uint_fast32_t typ_ref { static_cast<std::uint_fast32_t>(-1) }; //<- to be inferred

  std::vector<std::uint_fast32_t> additional_references; // Stores an explicit position in the vector
};

struct hx_per_statement_ir
{
  std::vector<IRNodeKind> kinds;
  std::vector<std::uint_fast32_t> references; // stores the number of arguments per kind
  std::vector<IRData> data;

  struct free_variable
  {
    std::uint_fast32_t potentially_bounded_ref { static_cast<std::uint_fast32_t>(-1) };
    std::vector<std::uint_fast32_t> refs;
  };
  symbol_map<free_variable> free_variable_roots;

  std::shared_ptr<symbol> node_name; // used for assign or type assign, otherwise it's the empty symbol


  std::vector<std::shared_ptr<type>> types;

  void print(std::ostream& os) const
  {
    assert(!kinds.empty() && "IR cannot be zero, there is never an empty module!");
    print_subnode(os, 0);
  }
  std::uint_fast32_t print_subnode(std::ostream& os, std::uint_fast32_t node) const
  {
    switch(kinds[node])
    {
      case IRNodeKind::undef:      os << "undef"; break;
      case IRNodeKind::literal:    os << data[node].symb.get_string(); break;
      case IRNodeKind::unit:       os << "()"; break;
      case IRNodeKind::top:        os << "TOP"; break;
      case IRNodeKind::bot:        os << "BOT"; break;
      case IRNodeKind::app:        { os << "("; node = print_subnode(os, node + 1);
                                     os << ") ("; node = print_subnode(os, node + 1);
                                     os << ")"; } break;
      case IRNodeKind::access:     { os << "("; node = print_subnode(os, node + 1);
                                     os << ").("; node = print_subnode(os, node + 1);
                                     os << ")"; } break;
      case IRNodeKind::tuple:      {
        os << "(";
        auto cpy = node;
        for(std::uint_fast32_t i = 1; i <= references[cpy]; ++i)
        {
          node = print_subnode(os, node + 1);
          if(i < references[cpy])
            os << " , ";
        }
        os << ")";
      } break;
      case IRNodeKind::lambda:     { os << "\\"; node = print_subnode(os, node + 1);
                                     os << "."; node = print_subnode(os, node + 1); } break;
      case IRNodeKind::param:      os << data[node].symb; break;
      case IRNodeKind::match:      { node = print_subnode(os, node + 1); os << " => ";
                                     node = print_subnode(os, node + 1); } break;
      case IRNodeKind::pattern:    node = print_subnode(os, node + 1); break;
      case IRNodeKind::pattern_matcher: {
        auto cpy = node;
        os << "case (";
        node = print_subnode(os, node + 1);
        os << ") [";
        for(std::uint_fast32_t i = 1; i < references[cpy]; ++i)
        {
          node = print_subnode(os, node + 1);
          if(i + 1 < references[cpy])
            os << " | ";
        }
        os << "]";
      } break;
      case IRNodeKind::identifier: os << data[node].symb; break;
      case IRNodeKind::block:      {
        os << "{";
        auto cpy = node;
        for(std::uint_fast32_t i = 1; i <= references[cpy]; ++i)
        {
          node = print_subnode(os, node + 1);
          if(i < references[cpy])
            os << " ; ";
        }
        os << "}";
      } break;
      case IRNodeKind::assign:     { os << node_name->get_string() << " = ";
                                     node = print_subnode(os, node + 1); os << ";"; } break;
      case IRNodeKind::assign_type: {
        os << "type " << node_name->get_string() << " = ";
        const auto& typ = *types[data[node].typ_ref];
        auto cpy = node;
        if(typ.is_inductive())
        {
          auto& ctyp = static_cast<const inductive&>(typ);

          for(auto it = ctyp.constructors.begin(); it != ctyp.constructors.end(); ++it)
          {
            const auto* c = (*it).get();
            if(c->is_identifier())
            {
              const auto* cc = static_cast<const constructors::identifier*>(c);

              os << cc->id.get_string();
            }
            else if(c->is_app())
            {
              const auto& cc = static_cast<const constructors::app*>(c);
              
              const auto& ca = static_cast<const constructors::identifier*>(cc->caller.get());

              // TODO: we need to visit the type here recursively!
              const auto& cp = static_cast<const constructors::identifier*>(cc->param.get());

              os << ca->id.get_string() << " " << cp->id.get_string();
            }
            else if(c->is_unit())
            {
              os << "()";
            }
            else
              assert(false && "WIP. Operation currently unsupported.");

            if(std::next(it) != ctyp.constructors.end())
              os << " | ";
          }
        }
        else
          assert(false && "We don't support any other types besides inductive ones.");
        node = node - references[cpy];
        os << ";";
      } break;
      case IRNodeKind::expr_stmt:  { node = print_subnode(os, node + 1); os << ";"; } break;
      case IRNodeKind::binary_exp: { auto cpy = node; os << "(("; node = print_subnode(os, node + 1); os << ") ";
                                     os << data[cpy].symb.get_string();
                                     os << " ("; node = print_subnode(os, node + 1); os << "))"; } break;
    }
    return node;
  }
};

