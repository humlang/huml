#include <per_statement_ir.hpp>
#include <types.hpp>

std::uint_fast32_t hx_per_statement_ir::print_subnode(std::ostream& os, std::uint_fast32_t node) const
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

