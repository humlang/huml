#include <backend/builder.hpp>

#include <ostream>
#include <string>
#include <queue>

ir::builder::builder()
{
  // Bootstrap
  kind(); type(); prop();
  unit();
}

ir::Node::cRef ir::builder::root(const std::vector<ir::Node::cRef>& nodes)
{
  return lookup_or_emplace(Node::mk_node<Root>(nodes));
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

ir::Node::cRef ir::builder::rec()
{ return lookup_or_emplace(Node::mk_node<Constructor>("_Z", nullptr)); }

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

  if(type != nullptr && type->kind() == NodeKind::Fn && !type->to<Fn>()->is_external())
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

  return to_ret->set_type(expr->type());
}

ir::Node::cRef ir::builder::lit(std::uint_fast64_t value)
{
  auto lit32 = lookup_or_emplace(Node::mk_node<Literal>(32));
  auto to_ret = lookup_or_emplace(Node::mk_node<Literal>(value))->set_type(i(false, lit32));

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

  return to_ret;
}

ir::Node::cRef ir::builder::i(bool no_sign, Node::cRef size)
{
  auto ictor = lookup_or_emplace(Node::mk_node<Constructor>(no_sign ? "u" : "i", type()));
  auto to_ret = app(ictor, {size});

  return to_ret;
}

ir::Node::cRef ir::builder::ptr(Node::cRef from)
{
  auto p = lookup_or_emplace(Node::mk_node<Constructor>("_Ptr", type()));
  auto to_ret = app(p, {from});

  return to_ret;
}

ir::Node::cRef ir::builder::entry_ret()
{
  auto Int = i(false, lit(32));

  auto retf = fn({param(Int)}, bot());
  retf->make_external("__start_return");
  auto ret = param(retf);

  return ret;
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

  return to_ret;
}

ir::Node::cRef ir::builder::app(Node::cRef caller, std::vector<Node::cRef> args)
{
  bool not_null = std::all_of(args.begin(), args.end(), [](Node::cRef x) { return x != nullptr; });
  assert(caller != nullptr && not_null && "caller and arg must stay valid.");

  auto to_ret = lookup_or_emplace(Node::mk_node<App>(caller, args));

  // app does not return
  return to_ret->set_type(bot());
}

ir::Node::cRef ir::builder::destruct(Node::cRef of, std::vector<std::pair<Node::cRef, Node::cRef>> match_arms)
{
  // case has no value
  auto to_ret = lookup_or_emplace(Node::mk_node<Case>(of, match_arms))->set_type(bot());

  return to_ret;
}

ir::Node::Ref ir::builder::lookup_or_emplace(Node::Store store)
{
  store->mach_ = this;
  store->gid_ = gid++;

  if(auto it = nodes.find(store); it != nodes.end())
      return data[it->second].get(); // <- might be different pointer

  data.emplace_back(std::move(store));
  nodes.emplace(data.back().get(), data.size() - 1);

  return data.back().get();
}

bool ir::builder::is_free(ir::Node::cRef what, ir::Node::cRef in)
{
  // TODO: also check type
  static constexpr NodeComparator cmp;
  
  if(cmp(what, in))
    return false;

  for(std::size_t i = 0, e = in->kind() == NodeKind::Fn ? 1 : in->argc(); i < e; ++i)
    if(!is_free(what, in->me()[i]))
      return false;
  return true;
}

ir::Node::cRef ir::builder::subst(ir::Node::cRef what, ir::Node::cRef with, ir::Node::cRef in)
{
  // TODO: also subst type
  static constexpr ir::NodeComparator cmp;

  if(cmp(what, in))
    return with;

  auto it = nodes.find(in);
  assert(it != nodes.end() && "Node must belong to this builder");

  for(std::size_t i = 0, e = in->kind() == NodeKind::Fn ? 1 : in->argc(); i < e; ++i)
  {
    // We literally change where this node points to. This is the only place where we do these kinds of stateful things!
    Node::cRef old = data[it->second]->children_[i];

    Node::cRef fresh = subst(what, with, in->me()[i]);

    if(old != fresh)
    {
      // SUBST!
      // rehash this single element

      std::size_t pos = it->second;
      //nodes.erase(it);
      data[pos]->children_[i] = fresh;
      nodes.emplace(in, pos);

      assert(nodes.contains(in) && "Node must be re-hashed by now.");
    }
  }
  return in;
}

std::ostream& ir::builder::print(std::ostream& os, Node::cRef ref)
{
  if(ref->kind() != NodeKind::Fn)
    return os;
  NodeSet definitions_to_print;
  NodeSet defs_printed;
  auto collect_definitions = [&defs_printed,&definitions_to_print](auto self, Node::cRef ref) -> void
  {
    if(ref->kind() == NodeKind::Fn)
      definitions_to_print.emplace(ref);

    if(defs_printed.contains(ref))
      return;
    defs_printed.emplace(ref);
    for(std::size_t i = 0, e = ref->argc(); i < e; ++i)
      self(self, ref->me()[i]);
  };
  collect_definitions(collect_definitions, ref);

  auto internal = [&os](auto internal, Node::cRef ref) -> std::ostream&
  {
    switch(ref->kind())
    {
    case NodeKind::Kind: os << "Kind"; break;
    case NodeKind::Type: os << "Type"; break;
    case NodeKind::Prop: os << "Prop"; break;
    case NodeKind::Ctr:  os << "c" << ref->to<Constructor>()->name.get_string(); break;
    case NodeKind::Literal: os << ref->to<Literal>()->literal; break;
    case NodeKind::Param: os << "p" << ref->gid(); break;
    case NodeKind::ConstexprAnnot: os << "@"; internal(internal, ref->to<ConstexprAnnot>()->what()); break;
    case NodeKind::Unit: os << "()"; break;
    case NodeKind::Binary: {
        auto bin = ref->to<Binary>();

        std::string op;
        switch(bin->op)
        {
        case BinaryKind::Mult:  op = " * "; break;
        case BinaryKind::Plus:  op = " + "; break;
        case BinaryKind::Minus: op = " - "; break;
        }
        internal(internal, bin->lhs()) << op;
        internal(internal, bin->rhs());
      } break;
    case NodeKind::App: {
        auto ap = ref->to<App>();

        internal(internal, ap->caller()) << " (";
        std::size_t i = 0;
        for(auto& v : ap->args())
          internal(internal, v) << (++i < ap->argc() - 1 ? ", " : "");
        os << ")";
      } break;
    case NodeKind::Fn: {
        auto fn = ref->to<Fn>();
        os << "fn_" + std::to_string(fn->gid());
      } break;
    case NodeKind::Case: {
        auto cs = ref->to<Case>();

        auto vals = cs->match_arms();
        os << "case ";
        internal(internal, cs->of()) << "[ \n";
        std::size_t i = 0; 
        for(auto& v : vals)
        {
          os << ((i+=2) < cs->argc() - 1 ? "   | " : "     ");
          internal(internal, v.first)  << " => ";
          internal(internal, v.second) << "\n";
        }
        os << "    ]\n";
      } break;
    case NodeKind::Tup: {
        auto tup = ref->to<Tup>();

        os << "(";
        std::size_t i = 0;
        for(auto& v : tup->elements())
          internal(internal, v) << (++i < tup->argc() - 1 ? ", " : "");
        os << ")";
      } break;
    }
    return os;
  };
  for(auto it = definitions_to_print.begin(); it != definitions_to_print.end(); ++it)
  {
    auto fn = it.key()->to<Fn>();

    os << "fn_" + std::to_string(fn->gid()) << " #(";
    std::size_t i = 0;
    for(auto& v : fn->args())
    {
      internal(internal, v) << " : ";
      internal(internal, v->type()) << (++i < fn->argc() - 1 ? ", " : "");
    }
    os << ") -> ⊥:\n      ";
    internal(internal, fn->bdy()) << "\n\n";
  }
  return os << "\n\n";
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
    case NodeKind::Ctr:  os << "ctor_" << ref->to<Constructor>()->name.get_string(); break;
    case NodeKind::Literal: os << "l" << ref->to<Literal>()->literal; break;
    case NodeKind::Param: os << "p" << ref->gid(); break;
    case NodeKind::ConstexprAnnot: os << "inl_"; internal(internal, ref->to<ConstexprAnnot>()->what()); break;
    case NodeKind::Unit: os << "UNIT"; break;
    case NodeKind::Binary: {
        auto bin = ref->to<Binary>();

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
          defs_printed.insert(ref);
          internal(internal, bin->lhs()) << " -> " << op << ";\n";
          internal(internal, bin->rhs()) << " -> " << op << ";\n";
        }
        os << op;
      } break;
    case NodeKind::App: {
        auto ap = ref->to<App>();

        std::string op = "app_" + std::to_string(ap->gid());
        if(!defs_printed.contains(ref))
        {
          defs_printed.insert(ref);
          internal(internal, ap->caller()) << " -> " << op << ";\n";
          for(auto& v : ap->args())
            internal(internal, v) << " -> " << op << ";\n";
        }
        os << op;
      } break;
    case NodeKind::Fn: {
        auto fn = ref->to<Fn>();

        std::string op = "fn_" + std::to_string(fn->gid());
        if(!defs_printed.contains(ref))
        {
          defs_printed.insert(ref);
          for(auto& v : fn->args())
            internal(internal, v) << " -> " << op << ";\n";
          internal(internal, fn->bdy()) << " -> " << op << ";\n";
        }
        os << op;
      } break;
    case NodeKind::Case: {
        auto cs = ref->to<Case>();

        std::string op = "case_" + std::to_string(cs->gid());
        if(!defs_printed.contains(ref))
        {
          defs_printed.insert(ref);
          auto vals = cs->match_arms();
          internal(internal, cs->of()) << " -> " << op << ";\n";
          for(auto& v : vals)
          {
            internal(internal, v.first)  << " -> " << op << ";\n";
            internal(internal, v.second) << " -> " << op << ";\n";
          }
        }
        os << op;
      } break;
    case NodeKind::Tup: {
        auto tup = ref->to<Tup>();

        std::string op = "tup_" + std::to_string(tup->gid());
        if(!defs_printed.contains(ref))
        {
          defs_printed.insert(ref);
          for(auto& v : tup->elements())
            internal(internal, v) << " -> " << op << ";\n";
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

