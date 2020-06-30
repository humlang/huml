#include <backend/defunc.hpp>
#include <backend/builder.hpp>
#include <backend/node.hpp>

namespace ir
{

const Node* defunctionalize(builder& bb, const Node* n)
{
  NodeSet todo;
  NodeSet callsites;

  builder b;

  // The following does:
  //  1) Replace arguments that are literal functions (i.e. it is NodeKind::Fn) with their GID
  //     Also, store the caller and its arg if above applies inside callsites.
  //  2) Store any function in todo that needs a new body
  auto inner = [&todo,&callsites,&b](auto inner, Node::cRef n) -> Node::cRef
  {
    switch(n->kind())
    {
    case NodeKind::Binary: return b.binop(n->to<Binary>()->op, inner(inner, n->to<Binary>()->lhs()),
                                                               inner(inner, n->to<Binary>()->rhs())); break;
    case NodeKind::App:    {
        Node::cRef cal = inner(inner, n->to<App>()->caller());
        Node::cRef arg;
        // We don't want arg()->type()->kind(), as this would also consider params, but these are substituted with an
        // integer param anyway, so we only want to replace a function argument with its GID
        const bool higher = n->to<App>()->arg()->kind() == NodeKind::Fn;

        arg = higher ? b.lit(n->to<App>()->arg()->gid()) : inner(inner, n->to<App>()->arg());

        auto cl = b.app(cal, arg);
        if(higher)
          callsites.insert(cl);
        return cl;
      } break;
    case NodeKind::Fn:     {
        auto to_ret = b.fn(inner(inner, n->to<Fn>()->arg()), inner(inner, n->to<Fn>()->bdy()));

        // If arg() is a function, it becomes a simple integer. So, we want bdy() to become
        // a big switch that goes over all possible instantiations of our arg.
        // However, we cannot do this atm and need to delay the replacement of the body for later.
        if(n->to<Fn>()->arg()->type()->kind() == NodeKind::Fn)
          todo.insert(to_ret);
        return to_ret;
      } break;
    case NodeKind::Case:   {
        auto of = inner(inner, n->to<Case>()->of());
        std::vector<std::pair<Node::cRef, Node::cRef>> w;
        for(auto& v : n->to<Case>()->match_arms())
          w.emplace_back(inner(inner, v.first), inner(inner, v.second));
        return b.destruct(of, w);
      } break;
    case NodeKind::Param:  { // <- params can also be Fns, but we do not store them, as we want to get rid of these!
        // do not clone higher order params, replace them with fresh integer params!
        if(n->type()->kind() == NodeKind::Fn)
          return b.param(b.i(true, b.lit(32)));
        return n->clone(b);
      } break;
    }
    return n->clone(b);
  };
  auto new_n = inner(inner, n);

  // TODO: what is left to do is to iterate over todo and fix the functions bodies

  // We do this by cloning all of b into bb. TODO This can be made faster by adding a `subst` method to builder


  return new_n;
}

}

