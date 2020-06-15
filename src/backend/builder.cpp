#include <backend/builder.hpp>

#include <ostream>
#include <string>
#include <queue>


ir::Node::Ref ir::builder::kind()
{ return lookup_or_emplace(Node::mk_node<Kind>()); }

ir::Node::Ref ir::builder::type()
{ return lookup_or_emplace(Node::mk_node<Type>()); }

ir::Node::Ref ir::builder::prop()
{ return lookup_or_emplace(Node::mk_node<Prop>()); }

ir::Node::Ref ir::builder::id(symbol symb, Node::Ref type)
{
  assert(type != Node::no_ref && "Type must exist.");
  return lookup_or_emplace(Node::mk_node<Constructor>(symb, type));
}

ir::Node::Ref ir::builder::param(Node::Ref type)
{ return lookup_or_emplace(Node::mk_node<Param>(type)); }

ir::Fn::Ref ir::builder::fn(Node::Ref domain, Node::Ref codomain)
{
  assert(domain != nullptr && codomain != nullptr && "(co-)domain must stay valid.");
  return static_cast<Fn::Ref>(lookup_or_emplace(Node::mk_node<Fn>(domain, codomain)));
}

ir::Fn::Ref ir::builder::fn()
{ return static_cast<Fn::Ref>(lookup_or_emplace(Node::mk_node<Fn>())); }

ir::Node::Ref ir::builder::app(Node::Ref caller, Node::Ref callee)
{
  assert(caller != nullptr && callee != nullptr && "(co-)domain must stay valid.");
  return lookup_or_emplace(Node::mk_node<App>(caller, callee));
}

ir::Node::Ref ir::builder::destruct(Node::Ref of, std::vector<std::pair<Node::Ref, Node::Ref>> match_arms)
{ return lookup_or_emplace(Node::mk_node<Case>(of, match_arms)); }

ir::Node::Ref ir::builder::lookup_or_emplace(Node::Store store)
{
  if(auto it = nodes.find(store); it != nodes.end())
      return it->get(); // <- might be different pointer
  return nodes.emplace(std::move(store)).first->get();
}

std::ostream& ir::builder::print(std::ostream& os, Node::Ref ref)
{
  NodeMap<std::string> ns;
  std::size_t param_count = 0;
  std::size_t fn_count = 0;

  std::uint_fast16_t defining = 0;

  std::queue<Node::Ref> defs_to_print;
  defs_to_print.push(ref);

  auto y = [&os, &ns, &param_count, &fn_count, &defining, &defs_to_print](Node::Ref ref) -> std::ostream&
  {
    auto inner = [&os, &ns, &param_count, &fn_count, &defining, &defs_to_print](auto&& y, Node::Ref ref) -> std::ostream&
    {
      switch(ref->kind)
      {
      case NodeKind::Kind: return os << "Kind"; break;
      case NodeKind::Type: return os << "Type"; break;
      case NodeKind::Prop: return os << "Prop"; break;

      case NodeKind::Ctr: {
          return os << static_cast<Constructor::Ref>(ref)->name.get_string();
        } break;

      case NodeKind::Case: {
          Case::Ref cs = static_cast<Case::Ref>(ref);
          os << "case (";
          y(y, cs->of()) << ") [ ";

          const auto& arms = cs->match_arms();
          for(auto it = arms.begin(); it != arms.end(); ++it)
          {
            y(y, it->first) << " => ";
            y(y, it->second);
            if(std::next(it) != arms.end())
              os << " | ";
          }
          return os << " ]";
        } break;

      case NodeKind::Param: {
          if(auto it = ns.find(ref); it != ns.end())
            return os << it->second;

          auto name = "p" + std::to_string(param_count++);
          ns.emplace(ref, name);

          if(ref->type == Node::no_ref)
            return os << name;
          os << "(" << name << ") : ";
          return y(y, ref->type);
        } break;

      case NodeKind::App: {
          App::Ref app = static_cast<App::Ref>(ref);

          os << "(";
          y(y, app->caller()) << " ";
          return y(y, app->callee()) << ")";
        } break;

      case NodeKind::Fn: {
          Fn::Ref lm = static_cast<Fn::Ref>(ref);

          auto it = ns.find(lm);
          if(it != ns.end() && defining > 0)
            return os << it->second;
          auto name = it != ns.end() ? it->second : "f" + std::to_string(fn_count++);

          if(it == ns.end())
            ns.emplace(ref, name);

          if(defining > 0)
          {
            defs_to_print.push(ref);
            return os << "goto " << name;
          }
          defining++;

          os << name << " = \\(";
          y(y, lm->arg()) << "). ";
          auto& ret = y(y, lm->bdy());

          defining--;
          return ret;
        } break;
      }
      assert(false && "Unhandled case.");
      return os;
    };

    return inner(inner, ref);
  };

  while(!defs_to_print.empty())
  {
    y(defs_to_print.front()) << ";\n";
    defs_to_print.pop();
  }
  return os;
}

