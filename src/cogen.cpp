#include <cogen.hpp>
#include <backend/node.hpp>
#include <backend/builder.hpp>
#include <memory>

struct CoGen
{
  CoGen(ir::builder& mach)
    : mach(mach)
  {  }

  ir::Node::cRef cogen(identifier::ptr id)
  {
    assert(id->irn != ir::Node::no_ref && "free variables can't be lowered");

    return id->irn;
  }

  ir::Node::cRef cogen(number::ptr n)
  { return mach.lit(std::stoull(n->symb.get_string())); }

  ir::Node::cRef cogen(pointer::ptr p)
  { return mach.ptr(dispatch(p->of)); }

  ir::Node::cRef cogen(unit::ptr u)
  { return mach.unit(); }

  ir::Node::cRef cogen(kind::ptr k)
  { return mach.kind(); }

  ir::Node::cRef cogen(type::ptr t)
  { return mach.type(); }

  ir::Node::cRef cogen(prop::ptr p)
  { return mach.prop(); }

  ir::Node::cRef cogen(lambda::ptr l, symbol name)
  {
    // TODO: what about recursion

    // 0. collect all curried params
    auto uncurried = l->uncurry();
    // 1. generate params and body
    std::vector<ir::Node::cRef> args;
    for(auto& x : uncurried.first)
      args.emplace_back(dispatch(x));
    auto bdy = dispatch(uncurried.second);

    // 2. Add return continuation, arg type is our return type, new return type is ⊥
    auto ret = (name == symbol("main") ? mach.entry_ret() : mach.fn({ dispatch(uncurried.second->type) }, mach.bot()));
    args.emplace_back(ret);

    auto lm = mach.fn(args, bdy);
    lm->make_external(name);

    return lm;
  }

  ir::Node::cRef cogen(app::ptr ab)
  {
    ast_ptr a = ab;
    std::vector<ir::Node::cRef> args;
    while(a->kind == ASTNodeKind::app)
    {
      args.emplace_back(dispatch(std::static_pointer_cast<app>(a)->rhs));

      a = std::static_pointer_cast<app>(a)->lhs;
    }
    return mach.app(dispatch(a), args);
  }

  ir::Node::cRef cogen(pattern_matcher::ptr p)
  {
    std::vector<std::pair<ir::Node::cRef, ir::Node::cRef>> arms;
    for(auto& d : p->data)
    {
      assert(d->kind == ASTNodeKind::match && "match arm must be a match");

      match& m = static_cast<match&>(*d);

      arms.emplace_back(dispatch(m.pat), dispatch(m.exp));
    }
    return mach.destruct(dispatch(p->to_match), std::move(arms));
  }

  ir::Node::cRef cogen(assign::ptr a)
  {
    assert(false && "Unimplemented");

    // TODO: also consider lambdas
  }

  ir::Node::cRef cogen(expr_stmt::ptr e)
  { return dispatch(e); }

  ir::Node::cRef cogen(ast_ptr)
  {
    assert(false && "Cannot cogen this node");
  }


  ir::Node::cRef run(ast_ptr root)
  {
    return dispatch(root);
  }

  ir::Node::cRef dispatch(ast_ptr node)
  {
    if(auto it = seen_nodes.find(node); it != seen_nodes.end())
      return it->second;
    ir::Node::cRef to_ret = nullptr;
    switch(node->kind)
    {
    default: assert(false && "unhandled case");

    case ASTNodeKind::Kind: to_ret = cogen(std::static_pointer_cast<kind>(node));
    case ASTNodeKind::Prop: to_ret = cogen(std::static_pointer_cast<prop>(node));
    case ASTNodeKind::Type: to_ret = cogen(std::static_pointer_cast<type>(node));
    case ASTNodeKind::unit: to_ret = cogen(std::static_pointer_cast<unit>(node));
    case ASTNodeKind::number: to_ret = cogen(std::static_pointer_cast<number>(node));
    case ASTNodeKind::ptr: to_ret = cogen(std::static_pointer_cast<pointer>(node));
    case ASTNodeKind::assign: to_ret = cogen(std::static_pointer_cast<assign>(node));
    case ASTNodeKind::assign_data: to_ret = cogen(std::static_pointer_cast<assign_data>(node));
    case ASTNodeKind::assign_type: to_ret = cogen(std::static_pointer_cast<assign_type>(node));
    case ASTNodeKind::expr_stmt: to_ret = cogen(std::static_pointer_cast<expr_stmt>(node));
    case ASTNodeKind::directive: to_ret = cogen(std::static_pointer_cast<directive>(node));
    case ASTNodeKind::identifier: to_ret = cogen(std::static_pointer_cast<identifier>(node));
    case ASTNodeKind::lambda: to_ret = cogen(std::static_pointer_cast<lambda>(node));
    case ASTNodeKind::app: to_ret = cogen(std::static_pointer_cast<app>(node));
    case ASTNodeKind::match: to_ret = cogen(std::static_pointer_cast<match>(node));
    case ASTNodeKind::pattern_matcher: to_ret = cogen(std::static_pointer_cast<pattern_matcher>(node));
    }
    if(to_ret == nullptr)
      return cogen(node);
    return seen_nodes[node] = to_ret;
  }

private:
  ir::builder& mach;
  tsl::robin_map<ast_ptr, ir::Node::cRef> seen_nodes;
};


const ir::Node* cogen(ir::builder& mach, ast_ptr root)
{
  CoGen gen(mach);
  return gen.run(root);
}

