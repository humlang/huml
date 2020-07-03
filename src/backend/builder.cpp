#include <backend/builder.hpp>

#include <ostream>
#include <string>
#include <queue>

#include <iostream>

ir::builder::builder()
{
  // Bootstrap
  kind(); type(); prop();
  unit();
}

ir::Node::cRef ir::builder::kind()
{
  return lookup_or_emplace(Node::mk_node<Kind>())->set_type(
         lookup_or_emplace(Node::mk_node<Kind>()));
}

ir::Node::cRef ir::builder::type()
{ return lookup_or_emplace(Node::mk_node<Type>())->set_type(kind()); }

ir::Node::cRef ir::builder::prop()
{ return lookup_or_emplace(Node::mk_node<Prop>())->set_type(type()); }

ir::Node::cRef ir::builder::unit()
{ return lookup_or_emplace(Node::mk_node<Unit>())->set_type(type()); }

ir::Node::cRef ir::builder::tup(std::vector<Node::cRef> elems)
{
  std::vector<Node::cRef> el_typs;
  for(auto& arg : elems)
    el_typs.emplace_back(arg->type());

  auto to_ret = lookup_or_emplace(Node::mk_node<Tup>(elems));

  to_ret->set_type(lookup_or_emplace(Node::mk_node<Tup>(el_typs)));
  return to_ret;
}

ir::Node::cRef ir::builder::bot()
{ return lookup_or_emplace(Node::mk_node<Constructor>("_⊥", nullptr)); }

ir::Node::cRef ir::builder::id(symbol symb, Node::cRef type)
{
  assert(type != Node::no_ref && "Type must exist.");
  return lookup_or_emplace(Node::mk_node<Constructor>(symb, type));
}

ir::Node::cRef ir::builder::ignore() // TODO: replace nullptr with existential
{ return lookup_or_emplace(Node::mk_node<Constructor>("_", nullptr)); }

ir::Node::cRef ir::builder::param(Node::cRef type)
{ return lookup_or_emplace(Node::mk_node<Param>(type)); }

ir::Node::cRef ir::builder::lit(std::uint_fast64_t value)
{
  auto lit32 = lookup_or_emplace(Node::mk_node<Literal>(32));
  return lookup_or_emplace(Node::mk_node<Literal>(value))->set_type(i(false, lit32));
}

ir::Node::cRef ir::builder::binop(ir::BinaryKind op, Node::cRef lhs, Node::cRef rhs)
{
  // Preserve a certain order
  if(op != BinaryKind::Minus && lhs->kind() == NodeKind::Param && rhs->kind() != NodeKind::Param)
    std::swap(lhs, rhs);

  if(lhs->kind() == NodeKind::Literal)
  {
    auto li = static_cast<Literal::cRef>(lhs);
    switch(op)
    {
    case BinaryKind::Mult: {
        // 0 * n = 0
        if(li->literal == 0)
          return lit(0);
        // 1 * n = n
        else if(li->literal == 1)
          return rhs;
      } break;

    case BinaryKind::Plus: {
        // 0 + n = n
        if(li->literal == 0)
          return rhs;
      } break;
    }
  }
  else if(rhs->kind() == NodeKind::Literal)
  {
    auto ri = static_cast<Literal::cRef>(rhs);
    switch(op)
    {
    case BinaryKind::Mult: {
        // n * 0 = 0
        if(ri->literal == 0)
          return lit(0);
        // n * 1 = n
        else if(ri->literal == 1)
          return lhs;
      } break;

    case BinaryKind::Minus:
    case BinaryKind::Plus: {
        // n + 0 = n
        // n - 0 = n
        if(ri->literal == 0)
          return lhs;
      } break;
    }
  }
  // TODO: This does not work well if lhs->type() != rhs->type()
  return lookup_or_emplace(Node::mk_node<Binary>(op, lhs, rhs))->set_type(lhs->type());
}

ir::Node::cRef ir::builder::i(bool no_sign, Node::cRef size)
{
  auto ictor = lookup_or_emplace(Node::mk_node<Constructor>(no_sign ? "u" : "i", type()));
  return app(ictor, {size});
}

ir::Node::cRef ir::builder::ptr(Node::cRef from)
{
  auto p = lookup_or_emplace(Node::mk_node<Constructor>("_Ptr", type()));
  return app(p, {from});
}

ir::Fn::cRef ir::builder::fn(std::vector<Node::cRef> args, Node::cRef body)
{
  std::vector<Node::cRef> argTs;
  argTs.reserve(args.size());
  for(auto& v : args)
    argTs.emplace_back(v->type());

  return static_cast<Fn::cRef>(lookup_or_emplace(Node::mk_node<Fn>(args, body))
      ->set_type(lookup_or_emplace(Node::mk_node<Fn>(argTs, bot()))));
}

ir::Node::cRef ir::builder::app(Node::cRef caller, std::vector<Node::cRef> args)
{
  bool not_null = std::all_of(args.begin(), args.end(), [](Node::cRef x) { return x != nullptr; });

  assert(caller != nullptr && not_null && "caller and arg must stay valid.");
  return lookup_or_emplace(Node::mk_node<App>(caller, args));
}

ir::Node::cRef ir::builder::destruct(Node::cRef of, std::vector<std::pair<Node::cRef, Node::cRef>> match_arms)
{ return lookup_or_emplace(Node::mk_node<Case>(of, match_arms))->set_type(of->type()); }

ir::Node::Ref ir::builder::lookup_or_emplace(Node::Store store)
{
  store->mach_ = this;
  store->gid_ = gid++;

  if(auto it = nodes.find(store); it != nodes.end())
      return it->get(); // <- might be different pointer
  return nodes.emplace(std::move(store)).first->get();
}

ir::Node::cRef ir::builder::subst(ir::Node::cRef what, ir::Node::cRef with, ir::Node::cRef in)
{
  static constexpr ir::NodeComparator cmp;

  if(cmp(what, in))
    return with;

  auto& n = *in;
  std::size_t idx = 0;
  switch(n.kind())
  {
  case NodeKind::Kind:
  case NodeKind::Type:
  case NodeKind::Prop:
  case NodeKind::Unit:
  case NodeKind::Literal:
  case NodeKind::Ctr:
  case NodeKind::Param:
    goto end;

  case NodeKind::Binary:
  case NodeKind::Case:
  case NodeKind::App:
  case NodeKind::Tup:    {
      idx = 0;
      goto subst_children;
    };

  case NodeKind::Fn:     {
      // We start at 1 with functions, params bound by it should not be replaced.
       // TODO: do we need to do this for case as well?
      idx = 1;
      goto subst_children;
    } break;
  }
subst_children:
  for(std::size_t i = 0, e = n.argc(); i < e; ++i)
  {
    auto it = nodes.find(in);
    assert(it != nodes.end() && "Node must belong to this builder");

    // We literally change where this node points to. This is the only place where we do these kinds of stateful things!
    (*it)->children_[i] = subst(what, with, n[i]);
  }
end:
  return in;
}

std::ostream& ir::builder::print_graph(std::ostream& os, Node::cRef ref)
{
  ir::NodeSet defs_printed = {};

  auto internal = [&os, &defs_printed](auto internal, Node::cRef ref) -> std::ostream&
  {
    switch(ref->kind())
    {
    case NodeKind::Kind: os << "Kind"; break;
    case NodeKind::Type: os << "Type"; break;
    case NodeKind::Prop: os << "Prop"; break;
    case NodeKind::Ctr:  os << static_cast<Constructor::cRef>(ref)->name.get_string(); break;
    case NodeKind::Literal: os << static_cast<Literal::cRef>(ref)->literal; break;
    case NodeKind::Param: os << "p" << ref->gid(); break;
    case NodeKind::Unit: os << "UNIT"; break;
    case NodeKind::Binary: {
        auto bin = static_cast<Binary::cRef>(ref);

        std::string op;
        switch(bin->op)
        {
        case BinaryKind::Mult:  op = "mul"; break;
        case BinaryKind::Plus:  op = "pls"; break;
        case BinaryKind::Minus: op = "min"; break;
        }
        op += std::to_string(bin->gid());

        if(!defs_printed.contains(ref))
        {
          internal(internal, bin->lhs()) << " -> " << op << ";\n";
          internal(internal, bin->rhs()) << " -> " << op << ";\n";

          defs_printed.insert(ref);
        }
        os << op;
      } break;
    case NodeKind::App: {
        auto ap = static_cast<App::cRef>(ref);

        std::string op = "app_" + std::to_string(ap->gid());
        if(!defs_printed.contains(ref))
        {
          internal(internal, ap->caller()) << " -> " << op << ";\n";
          for(auto& v : ap->args())
            internal(internal, v) << " -> " << op << ";\n";

          defs_printed.insert(ref);
        }
        os << op;
      } break;
    case NodeKind::Fn: {
        auto fn = static_cast<Fn::cRef>(ref);

        std::string op = "fn_" + std::to_string(fn->gid());
        if(!defs_printed.contains(ref))
        {
          for(auto& v : fn->args())
            internal(internal, v) << " -> " << op << ";\n";
          internal(internal, fn->bdy()) << " -> " << op << ";\n";

          defs_printed.insert(ref);
        }
        os << op;
      } break;
    case NodeKind::Case: {
        auto cs = static_cast<Case::cRef>(ref);

        std::string op = "case_" + std::to_string(cs->gid());
        if(!defs_printed.contains(ref))
        {
          auto vals = cs->match_arms();
          internal(internal, cs->of()) << " -> " << op << ";\n";
          for(auto& v : vals)
          {
            internal(internal, v.first)  << " -> " << op << ";\n";
            internal(internal, v.second) << " -> " << op << ";\n";
          }

          defs_printed.insert(ref);
        }
        os << op;
      } break;
    }
    return os;
  };
  os << "digraph iea {\n";
  internal(internal, ref);
  return os << "}\n";
}

