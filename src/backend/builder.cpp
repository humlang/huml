#include <backend/builder.hpp>

#include <ostream>
#include <string>
#include <queue>

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

