#include <per_statement_ir.hpp>
#include <types.hpp>

std::uint_fast32_t hx_per_statement_ir::print_subnode(std::ostream& os, type_table& typtab, std::uint_fast32_t node) const
{
  switch(kinds[node])
  {
    case IRNodeKind::undef:      os << "undef"; break;
    case IRNodeKind::literal:    os << data[node].symb.get_string(); break;
    case IRNodeKind::unit:       os << "()"; break;
    case IRNodeKind::top:        os << "TOP"; break;
    case IRNodeKind::bot:        os << "BOT"; break;
    case IRNodeKind::Kind:       os << "Kind"; break;
    case IRNodeKind::Type:       os << "Type"; break;
    case IRNodeKind::Prop:       os << "Prop"; break;
    case IRNodeKind::type_check: { auto cpy = node; os << "(("; node = print_subnode(os, typtab, node + 1);
                                   os << ") : "; typtab.print_type(os, data[cpy].type_annot); os << ")"; } break;
    case IRNodeKind::app:        { os << "("; node = print_subnode(os, typtab, node + 1);
                                   os << ") ("; node = print_subnode(os, typtab, node + 1);
                                   os << ")"; } break;
    case IRNodeKind::access:     { os << "("; node = print_subnode(os, typtab, node + 1);
                                   os << ").("; node = print_subnode(os, typtab, node + 1);
                                   os << ")"; } break;
    case IRNodeKind::tuple:      {
      os << "(";
      auto cpy = node;
      for(std::uint_fast32_t i = 1; i <= references[cpy]; ++i)
      {
        node = print_subnode(os, typtab, node + 1);
        if(i < references[cpy])
          os << " , ";
      }
      os << ")";
    } break;
    case IRNodeKind::lambda:     { os << "\\"; node = print_subnode(os, typtab, node + 1);
                                   os << "."; node = print_subnode(os, typtab, node + 1); } break;
    case IRNodeKind::pi:         { os << "\\("; node = print_subnode(os, typtab, node + 1);
                                   os << " : "; node = print_subnode(os, typtab, node + 1);
                                   os << "). "; node = print_subnode(os, typtab, node + 1); } break;
    case IRNodeKind::param:      os << data[node].symb; break;
    case IRNodeKind::match:      { node = print_subnode(os, typtab, node + 1); os << " => ";
                                   node = print_subnode(os, typtab, node + 1); } break;
    case IRNodeKind::pattern:    node = print_subnode(os, typtab, node + 1); break;
    case IRNodeKind::pattern_matcher: {
      auto cpy = node;
      os << "case (";
      node = print_subnode(os, typtab, node + 1);
      os << ") [";
      for(std::uint_fast32_t i = 1; i < references[cpy]; ++i)
      {
        node = print_subnode(os, typtab, node + 1);
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
        node = print_subnode(os, typtab, node + 1);
        if(i < references[cpy])
          os << " ; ";
      }
      os << "}";
    } break;
    case IRNodeKind::assign:     { node = print_subnode(os, typtab, node + 1); os << " = ";
                                   node = print_subnode(os, typtab, node + 1); os << ";"; } break;
    case IRNodeKind::assign_type: {
      os << "type ";
      typtab.print_type(os, data[node].type_annot);
      os << ";";
    } break;
    case IRNodeKind::assign_data: {
      os << "data " << data[node].symb << " : ";
      typtab.print_type(os, data[node].type_annot);
      os << ";";
    } break;
    case IRNodeKind::expr_stmt:  { node = print_subnode(os, typtab, node + 1); os << ";"; } break;
    case IRNodeKind::binary_exp: { auto cpy = node; os << "(("; node = print_subnode(os, typtab, node + 1); os << ") ";
                                   os << data[cpy].symb.get_string();
                                   os << " ("; node = print_subnode(os, typtab, node + 1); os << "))"; } break;
  }
  return node;
}

