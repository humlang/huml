#include <backend/node.hpp>
#include <backend/builder.hpp>

ir::Node::Node(NodeKind kind, std::size_t argc)
    : kind_(kind), nominal_(true), argc_(argc), children_(), type_(no_ref), mach_(), gid_()
{ children_.resize(argc, nullptr); }

ir::Node::Node(NodeKind kind, std::vector<Node::Ref> children)
    : kind_(kind), nominal_(false), argc_(children.size()), children_(children), type_(no_ref), mach_(), gid_()
{  }

ir::NodeKind ir::Node::kind() const
{ return kind_; }
bool ir::Node::nominal() const
{ return nominal_; }
std::size_t ir::Node::argc() const
{ return argc_; }
std::uint_fast64_t ir::Node::gid() const
{ return gid_; }
ir::Node::Ref ir::Node::type() const
{ return type_; }

ir::Node::Ref& ir::Node::operator[](std::size_t idx)
{ return children_[idx]; }
const ir::Node::cRef& ir::Node::operator[](std::size_t idx) const
{ return children_[idx]; }

ir::Node& ir::Node::me()
{ return *this; }
const ir::Node& ir::Node::me() const
{ return *this; }

std::size_t ir::NodeHasher::operator()(const Node::cRef str) const
{
  std::size_t seed = (static_cast<std::size_t>(str->kind()) << 3) + str->argc();

  if(str->nominal())
    return seed ^ str->gid();

  for(std::size_t i = 0, e = str->argc(); i < e; ++i)
    seed ^= (*this)(str->me()[0]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

  // Add salts for nodes with additional data
  switch(str->kind())
  {
  case NodeKind::Binary: seed ^= static_cast<std::size_t>(static_cast<Binary::cRef>(str)->op) + 0x9e3779b9 + (seed << 6) + (seed >> 2); break;

  case NodeKind::Ctr: seed ^= static_cast<std::size_t>(static_cast<Constructor::cRef>(str)->name.get_hash()) + 0x9e3779b9 + (seed << 6) + (seed >> 2); break;

  case NodeKind::Literal: seed ^= static_cast<std::size_t>(static_cast<Literal::cRef>(str)->literal) + 0x9e3779b9 + (seed << 6) + (seed >> 2); break;

  case NodeKind::Int: seed ^= static_cast<std::size_t>(static_cast<Int::cRef>(str)->no_sign) + 0x9e3779b9 + (seed << 6) + (seed >> 2); break;
  }

  return seed;
}

bool ir::NodeComparator::operator()(const ir::Node::cRef lhs, const ir::Node::cRef rhs) const
{
  if(lhs->nominal() || rhs->nominal())
    return &lhs == &rhs;

  if(!(lhs->argc() == rhs->argc() && lhs->kind() == rhs->kind() && lhs->type() == rhs->type()))
    return false;
  for(std::size_t i = 0, e = lhs->argc(); i < e; ++i)
    if(lhs->me()[i] != rhs->me()[i])
      return false;
  return true;
}

