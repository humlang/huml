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
  const type* typ;

  std::vector<std::int_fast32_t> additional_references; // Stores an explicit position in the vector
};

struct hx_per_statement_ir
{
  std::vector<IRNodeKind> kinds;
  std::vector<std::int_fast32_t> references; // stores the number of arguments per kind
  std::vector<IRData> data;

  symbol_map<std::int_fast32_t> free_variable_roots;


  std::vector<type> types;

  void print(std::ostream& os) const
  {
    print_node(os, kinds.size() - 1);
  }
private:
  std::int_fast32_t print_node(std::ostream& os, std::int_fast32_t node) const
  {
    switch(kinds[node])
    {
      case IRNodeKind::undef:      os << "undef"; break;
      case IRNodeKind::literal:    os << data[node].symb.get_string(); break;
      case IRNodeKind::unit:       os << "()"; break;
      case IRNodeKind::top:        os << "TOP"; break;
      case IRNodeKind::bot:        os << "BOT"; break;
      case IRNodeKind::app:        { os << "("; node = print_node(os, node - 1);
                                     os << ") ("; node = print_node(os, node - 1);
                                     os << ")"; } break;
      case IRNodeKind::access:     { os << "("; node = print_node(os, node - 1);
                                     os << ").("; node = print_node(os, node - 1);
                                     os << ")"; } break;
      case IRNodeKind::tuple:      {
        os << "(";
        auto cpy = node;
        for(std::int_fast32_t i = 1; i <= references[cpy]; ++i)
        {
          node = print_node(os, node - 1);
          if(i + 1 < references[cpy])
            os << " , ";
        }
        os << ")";
      } break;
      case IRNodeKind::lambda:     { os << "\\"; node = print_node(os, node - 1);
                                     os << "."; node = print_node(os, node - 1); } break;
      case IRNodeKind::param:      os << data[node].symb; break;
      case IRNodeKind::match:      { node = print_node(os, node - 1); os << " => ";
                                     node = print_node(os, node - 1); } break;
      case IRNodeKind::pattern:    node = print_node(os, node - 1); break;
      case IRNodeKind::pattern_matcher: {
        auto cpy = node;
        os << "case (";
        node = print_node(os, node - 1);
        os << ") [";
        for(std::int_fast32_t i = 1; i < references[cpy]; ++i)
        {
          node = print_node(os, node - 1);
          if(i + 1 < references[cpy])
            os << " | ";
        }
        os << "]";
      } break;
      case IRNodeKind::identifier: os << data[node].symb; break;
      case IRNodeKind::block:      {
        os << "{";
        auto cpy = node;
        for(std::int_fast32_t i = 1; i <= references[cpy]; ++i)
        {
          node = print_node(os, node - 1);
          if(i + 1 < references[cpy])
            os << " ; ";
        }
        os << "}";
      } break;
      case IRNodeKind::assign:     { node = print_node(os, node - 1); os << " = ";
                                     node = print_node(os, node - 1); os << ";"; } break;
      case IRNodeKind::assign_type: os << "[TODO] assign_type [TODO]"; break;
      case IRNodeKind::expr_stmt:  { node = print_node(os, node - 1); os << ";"; } break;
      case IRNodeKind::binary_exp: { auto cpy = node; os << "(("; node = print_node(os, node - 1); os << ") ";
                                     os << data[cpy].symb.get_string();
                                     os << " ("; node = print_node(os, node - 1); os << "))"; } break;
    }
    return node;
  }
};

