#include <backend/node.hpp>
#include <backend/builder.hpp>

ir::Node::Node(NodeKind kind, std::vector<Node::cRef> children)
    : kind_(kind), children_(children), type_(no_ref), mach_(), gid_()
{  }

ir::NodeKind ir::Node::kind() const
{ return kind_; }

std::size_t ir::Node::argc() const
{ return children_.size(); }

std::uint_fast64_t ir::Node::gid() const
{ return gid_; }

symbol ir::Node::unique_name() const
{ return "n" + std::to_string(gid()); }

ir::Node::cRef ir::Node::type() const
{ return type_; }

const ir::Node::cRef& ir::Node::operator[](std::size_t idx) const
{ return children_[idx]; }

const ir::Node& ir::Node::me() const
{ return *this; }

std::size_t ir::NodeHasher::operator()(const Node::cRef str) const
{
  std::size_t seed = (static_cast<std::size_t>(str->kind()) << 3) + str->argc();

  for(std::size_t i = 0, e = str->argc(); i < e; ++i)
    seed ^= (*this)(str->me()[0]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

  // Add salts for nodes with additional data
  switch(str->kind())
  {
  case NodeKind::Binary:  seed ^= static_cast<std::size_t>(static_cast<Binary::cRef>(str)->op) + 0x9e3779b9 + (seed << 6) + (seed >> 2); break;

  case NodeKind::Ctr:     seed ^= static_cast<std::size_t>(static_cast<Constructor::cRef>(str)->name.get_hash()) + 0x9e3779b9 + (seed << 6) + (seed >> 2); break;

  case NodeKind::Literal: seed ^= static_cast<std::size_t>(static_cast<Literal::cRef>(str)->literal) + 0x9e3779b9 + (seed << 6) + (seed >> 2); break;

  case NodeKind::Param:   seed ^= str->gid() + 0x9e3779b9 + (seed << 6) + (seed >> 2); break;
  }

  return seed;
}

bool ir::NodeComparator::operator()(const ir::Node::cRef lhs, const ir::Node::cRef rhs) const
{
  if(!(lhs->argc() == rhs->argc() && lhs->kind() == rhs->kind() && lhs->type() == rhs->type()))
    return false;
  for(std::size_t i = 0, e = lhs->argc(); i < e; ++i)
    if(lhs->me()[i] != rhs->me()[i])
      return false;
  switch(lhs->kind())
  {
  case NodeKind::Binary:  return lhs->to<Binary>()->op == rhs->to<Binary>()->op;
  case NodeKind::Ctr:     return lhs->to<Constructor>()->name == rhs->to<Constructor>()->name;
  case NodeKind::Literal: return lhs->to<Literal>()->literal == rhs->to<Literal>()->literal;
  case NodeKind::Param:   return lhs->gid() == rhs->gid();
  }
  return true;
}

// TODO: this is dangerous! The idea of these methods is, that they clone themselves into the potentially different builder.
//       However, for `Param` and `Constructor` this is simply not possible.
ir::Node::cRef ir::Param::clone(ir::builder& b) const
{ return this; } // <- params are not cloned!
ir::Node::cRef ir::Constructor::clone(ir::builder& b) const
{ return this; } // <- nothing to clone, name won't change and it must be unique
ir::Node::cRef ir::Literal::clone(ir::builder& b) const
{ return b.lit(literal); }
ir::Node::cRef ir::ConstexprAnnot::clone(ir::builder& b) const
{ return b.cexpr(what()); }
ir::Node::cRef ir::Binary::clone(ir::builder& b) const
{ return b.binop(op, lhs()->clone(b), rhs()->clone(b)); }
ir::Node::cRef ir::Fn::clone(ir::builder& b) const
{
  auto new_bdy = bdy()->clone(b);

  auto argz = args();
  for(auto& v : argz)
  {
    Node::cRef old_v = v;

    // build new param
    v = b.param(old_v->type());

    // We need to subst the old param with the new param in the body
    new_bdy = b.subst(old_v, v, new_bdy);
  }
  return b.fn(argz, new_bdy);
}
ir::Node::cRef ir::App::clone(ir::builder& b) const
{
  auto argz = args();
  for(auto& v : argz)
    v = v->clone(b);
  return b.app(caller()->clone(b), argz);
}
ir::Node::cRef ir::Case::clone(ir::builder& b) const
{
  // TODO: if case patterns are binding we need to replace the old with the new bindings
  
  auto arms = match_arms();
  for(auto& arm : arms)
    arm = std::make_pair(arm.first->clone(b), arm.second->clone(b));
  return b.destruct(of()->clone(b), arms);
}
ir::Node::cRef ir::Kind::clone(ir::builder& b) const
{ return b.kind(); }
ir::Node::cRef ir::Type::clone(ir::builder& b) const
{ return b.type(); }
ir::Node::cRef ir::Prop::clone(ir::builder& b) const
{ return b.prop(); }
ir::Node::cRef ir::Unit::clone(ir::builder& b) const
{ return b.unit(); }
ir::Node::cRef ir::Tup::clone(ir::builder& b) const
{
  auto elms = elements();
  for(auto& el : elms)
    el = el->clone(b);
  return b.tup(elms);
}


