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
  auto apply = b.id("_App", nullptr); // <- defunctionalization's apply function

  // The following does:
  //  1) Replace arguments that are literal functions (i.e. it is NodeKind::Fn) with their GID
  //     Also, store the caller and its arg if above applies inside callsites.
  //  2) Store any function in todo that needs a new body
  auto inner = [&apply,&callsites,&todo,&b](auto inner, Node::cRef n) -> Node::cRef
  {
    switch(n->kind())
    {
    case NodeKind::Binary: return b.binop(n->to<Binary>()->op, inner(inner, n->to<Binary>()->lhs()),
                                                               inner(inner, n->to<Binary>()->rhs())); break;
    case NodeKind::App:    {
        Node::cRef cal = inner(inner, n->to<App>()->caller());
        // We don't want arg()->type()->kind(), as this would also consider params, but these are substituted with an
        // integer param anyway, so we only want to replace a function argument with its GID
        const bool higher = n->to<App>()->arg()->kind() == NodeKind::Fn;

        Node::cRef arg = higher ? b.lit(n->to<App>()->arg()->gid()) : inner(inner, n->to<App>()->arg());

        // ALL calls go through the apply function
        auto ap = b.app(apply, b.tup({ cal, arg }));
        callsites.insert(ap);
        return ap;
      } break;
    case NodeKind::Fn:     {
        const bool ext = n->to<Fn>()->is_external(); // <- we should not change "higher order" external functions, e.g. C's signal
        auto to_ret = b.fn(ext ? n->to<Fn>()->arg()->clone(b) : inner(inner, n->to<Fn>()->arg()),
                           inner(inner, n->to<Fn>()->bdy()));
        // If arg() is a function, it becomes a simple integer. So, we want bdy() to become
        // a big switch that goes over all possible instantiations of our arg.
        // However, we cannot do this atm and need to delay the replacement of the body for later.
        if(n->to<Fn>()->arg()->type()->kind() == NodeKind::Fn && !ext)
          todo.insert(to_ret);

        // TODO: do we need to handle stuff such as `plus 5`?
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

  // what is left to do is to iterate over todo and fix the functions bodies
  // We do this by cloning all of b into bb. TODO This can be made faster by adding a `subst` method to builder
  for(auto& v : todo)
  {
    auto fn = v->to<Fn>();
    assert(fn->arg()->type() == bb.i(true, b.lit(32)) && "Arguments must not be functions anymore.");

    // Replace the body with a case that chooses between all possible call sites
    auto cs = bb.destruct(fn->arg()
  }

  auto caseify = [&todo, &callsites, &bb](auto caseify, const Node* n) -> Node::cRef
  {
    switch(n->kind())
    {
    case NodeKind::Fn: {

      } break;
    case NodeKind::App: {

      } break;
    case NodeKind::Binary: return bb.binop(n->to<Binary>()->op, caseify(caseify, n->to<Binary>()->lhs()),
                                                                caseify(caseify, n->to<Binary>()->rhs())); break;
    case NodeKind::Case: {
        auto of = caseify(caseify, n->to<Case>());
        std::vector<std::pair<Node::cRef, Node::cRef>> w;
        for(auto& v : n->to<Case>()->match_arms())
          w.emplace_back(caseify(caseify, v.first), caseify(caseify, v.second));
        return bb.destruct(of, w);
      } break;
    }
    return node->clone(bb);
  };

  return caseify(caseify, new_n);
}

}

