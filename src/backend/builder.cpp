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

  for(auto& a : elems)
    uses_of[a].emplace(to_ret);

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
{
  auto to_ret = lookup_or_emplace(Node::mk_node<Param>(type));

  if(type != nullptr && type->kind() == NodeKind::Fn)
  {
    // if we have a function as param, we want to specialize it for codegen
    return cexpr(to_ret);
  }
  return to_ret;
}

ir::Node::cRef ir::builder::cexpr(Node::cRef expr)
{
  assert(expr->kind() == NodeKind::Param && "Annotations only works with params.");

  auto to_ret = lookup_or_emplace(Node::mk_node<ConstexprAnnot>(expr));
  uses_of[expr].emplace(to_ret);

  return to_ret;
}

ir::Node::cRef ir::builder::lit(std::uint_fast64_t value)
{
  auto lit32 = lookup_or_emplace(Node::mk_node<Literal>(32));
  auto to_ret = lookup_or_emplace(Node::mk_node<Literal>(value))->set_type(i(false, lit32));

  uses_of[lit32].emplace(to_ret);

  return to_ret;
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
  auto to_ret = lookup_or_emplace(Node::mk_node<Binary>(op, lhs, rhs))->set_type(lhs->type());

  uses_of[lhs].emplace(to_ret);
  uses_of[rhs].emplace(to_ret);

  return to_ret;
}

ir::Node::cRef ir::builder::i(bool no_sign, Node::cRef size)
{
  auto ictor = lookup_or_emplace(Node::mk_node<Constructor>(no_sign ? "u" : "i", type()));
  auto to_ret = app(ictor, {size});

  uses_of[ictor].emplace(to_ret);
  uses_of[size].emplace(to_ret);

  return to_ret;
}

ir::Node::cRef ir::builder::ptr(Node::cRef from)
{
  auto p = lookup_or_emplace(Node::mk_node<Constructor>("_Ptr", type()));
  auto to_ret = app(p, {from});

  uses_of[p].emplace(to_ret);
  uses_of[from].emplace(to_ret);

  return to_ret;
}

ir::Fn::cRef ir::builder::fn(std::vector<Node::cRef> args, Node::cRef body)
{
  assert(!args.empty() && "args must not be empty. Use a single unit argument for functions without arguments.");

  std::vector<Node::cRef> argTs;
  argTs.reserve(args.size());
  for(auto& v : args)
    argTs.emplace_back(v->type());

  auto to_ret = static_cast<Fn::cRef>(lookup_or_emplace(Node::mk_node<Fn>(args, body))
      ->set_type(lookup_or_emplace(Node::mk_node<Fn>(argTs, bot()))));

  for(auto& a : args)
    uses_of[a].emplace(to_ret);
  uses_of[body].emplace(to_ret);

  return to_ret;
}

ir::Node::cRef ir::builder::app(Node::cRef caller, std::vector<Node::cRef> args)
{
  bool not_null = std::all_of(args.begin(), args.end(), [](Node::cRef x) { return x != nullptr; });
  assert(caller != nullptr && not_null && "caller and arg must stay valid.");

  auto to_ret = lookup_or_emplace(Node::mk_node<App>(caller, args));

  uses_of[caller].emplace(to_ret);
  for(auto& a : args)
    uses_of[a].emplace(to_ret);

  // app does not return
  return to_ret->set_type(bot());
}

ir::Node::cRef ir::builder::destruct(Node::cRef of, std::vector<std::pair<Node::cRef, Node::cRef>> match_arms)
{
  // case has no value
  auto to_ret = lookup_or_emplace(Node::mk_node<Case>(of, match_arms))->set_type(bot());

  uses_of[of].emplace(to_ret);
  for(auto& p : match_arms)
  {
    uses_of[p.first].emplace(to_ret);
    uses_of[p.second].emplace(to_ret);
  }
  return to_ret;
}

ir::Node::Ref ir::builder::lookup_or_emplace(Node::Store store)
{
  store->mach_ = this;
  store->gid_ = gid++;

  if(auto it = nodes.find(store); it != nodes.end())
      return it->get(); // <- might be different pointer
  return nodes.emplace(std::move(store)).first->get();
}

ir::Node::cRef ir::builder::subst(ir::Node::cRef what, ir::Node::cRef with)
{
  // TODO: also subst type
  static constexpr NodeComparator cmp;
  
  // go through all uses_of and simply change the value
  for(auto& in : uses_of[what])
  {
    auto it = nodes.find(in);
    assert(it != nodes.end() && "Node must belong to this builder");

    // Now find the arg in the children_
    bool was_subst = false;
    for(std::size_t i = 0, e = in->argc(); i < e; ++i)
    {
      // we may not subst params in binding occurences
      if(in->kind() == NodeKind::Fn && i > 0)
        break;

      auto& c = (*it)->children_[i];
      if(cmp(c, what))
      {
        // SUBST!
        c = with;
        was_subst = true;
      }
    }
    assert(was_subst && "Node appears in uses_of set, so we should've substituted it!");
  }

  uses_of[with].insert(uses_of[what].begin(), uses_of[what].end());
  uses_of[what].clear();
  nodes.erase(what); // erase node, we substituted it, so it won't be used anymore

  return with;
}

ir::Node::cRef ir::builder::subst(ir::Node::cRef what, ir::Node::cRef with, ir::Node::cRef in)
{
  // TODO: also subst type
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
       // TODO: do we need to do kind of this for case patterns as well?
      idx = 1;
      goto subst_children;
    } break;
  }
subst_children:
  for(std::size_t i = idx, e = n.argc(); i < e; ++i)
  {
    auto it = nodes.find(in);
    assert(it != nodes.end() && "Node must belong to this builder");

    // We literally change where this node points to. This is the only place where we do these kinds of stateful things!
    Node::cRef old = (*it)->children_[i];

    (*it)->children_[i] = subst(what, with, n[i]);

    if(old != (*it)->children_[i])
    {
      /// it was substituted. we need to update uses_of set

      // remove old use
      assert(uses_of[old].find(in) != uses_of[old].end() && "We substituted, so the use should be at the end.");
      uses_of[old].erase(in);
      
      // insert new use
      uses_of[with].insert(in);
    }
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

