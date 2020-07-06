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
  if(lhs == rhs)
    return true;

  if(!(lhs->argc() == rhs->argc() && lhs->kind() == rhs->kind() && lhs->type() == rhs->type()))
    return false;
  if(lhs->kind() == NodeKind::ConstexprAnnot)
    return (*this)(lhs->to<ConstexprAnnot>()->what(), rhs->to<ConstexprAnnot>()->what());

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

ir::Node::cRef ir::Param::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{
  bool is_in_there = old_to_new.contains(this);
  assert(is_in_there && "can't clone params directly"); // <- params are not cloned!

  return old_to_new.find(this)->second;
}
ir::Node::cRef ir::Constructor::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{ return b.lookup_or_emplace(Node::mk_node<Constructor>(name, type())); }
ir::Node::cRef ir::Literal::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{ return b.lit(literal); }
ir::Node::cRef ir::ConstexprAnnot::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{
  auto res = what()->clone(b, old_to_new);

  assert(res->kind() == NodeKind::ConstexprAnnot && "Param must have been stored as cexprannot");

  return res;
}
ir::Node::cRef ir::Binary::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{ return b.binop(op, lhs()->clone(b, old_to_new), rhs()->clone(b, old_to_new)); }
ir::Node::cRef ir::Fn::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{
  if(auto it = old_to_new.find(this); it != old_to_new.end())
    return it->second;

  auto old_args = args();
  auto argz = args();
  for(auto& v : argz)
  {
    Node::cRef old_v = v;

    // build new param
    if(old_v->kind() == NodeKind::Param)
    {
      v = b.lookup_or_emplace(Node::mk_node<Param>(v->type()));

      assert(!old_to_new.contains(old_v) && "ill-formed IR");
      old_to_new.emplace(old_v, v);
    }
    else if(old_v->kind() == NodeKind::ConstexprAnnot)
    {
      v = b.lookup_or_emplace(Node::mk_node<Param>(v->type()));
      v = b.cexpr(v);

      assert(!old_to_new.contains(old_v) && "ill-formed IR");
      old_to_new.emplace(old_v, v);
      old_to_new.emplace(old_v->to<ConstexprAnnot>()->what(), v);
    }
    else
      assert(false && "Ill-formed IR");
  }
  // new_bdy will use old_to_new map to clone the param
  auto new_bdy = bdy()->clone(b, old_to_new);

  auto to_ret = b.fn(argz, new_bdy);
  old_to_new.emplace(this, to_ret);

  return to_ret;
}
ir::Node::cRef ir::App::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{
  auto argz = args();
  for(auto& v : argz)
    v = v->clone(b, old_to_new);
  return b.app(caller()->clone(b, old_to_new), argz);
}
ir::Node::cRef ir::Case::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{
  // TODO: if case patterns are binding we need to replace the old with the new bindings
  

  auto arms = match_arms();
  for(auto& arm : arms)
    arm = std::make_pair(arm.first->clone(b, old_to_new), arm.second->clone(b, old_to_new));
  return b.destruct(of()->clone(b, old_to_new), arms);
}
ir::Node::cRef ir::Kind::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{ return b.kind(); }
ir::Node::cRef ir::Type::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{ return b.type(); }
ir::Node::cRef ir::Prop::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{ return b.prop(); }
ir::Node::cRef ir::Unit::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{ return b.unit(); }
ir::Node::cRef ir::Tup::clone(ir::builder& b, NodeMap<Node::cRef>& old_to_new) const
{
  auto elms = elements();
  for(auto& el : elms)
    el = el->clone(b, old_to_new);
  return b.tup(elms);
}


