#include <backend/builder.hpp>

#include <ostream>
#include <string>

std::atomic_size_t ir::NodeHasher::param_count = std::atomic_size_t(1) << 63;
std::atomic_size_t ir::NodeHasher::lam_count = std::atomic_size_t(1) << 62;

std::ostream& ir::builder::print(std::ostream& os, Node::Ref ref)
{
  NodeMap<std::string> ns;
  std::size_t param_count = 0;
  std::size_t fn_count = 0;

  auto y = [&os, &ns, &param_count, &fn_count](Node::Ref ref) -> std::ostream&
  {
    auto inner = [&os, &ns, &param_count, &fn_count](auto&& y, Node::Ref ref) -> std::ostream&
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

          if(auto it = ns.find(lm); it != ns.end())
            return os << it->second;

          auto name = "f" + std::to_string(fn_count++);
          ns.emplace(ref, name);

          os << name << " = \\(";
          y(y, lm->arg()) << "). ";
          return y(y, lm->bdy());
        } break;
      }
      assert(false && "Unhandled case.");
      return os;
    };

    return inner(inner, ref);
  };
  return y(ref);
}

