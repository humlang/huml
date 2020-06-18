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

  auto bot = id("⊤", type());

  // "main" function's exit     () -> TOP
  world_exit = fn(unit(), bot);

  // "main" function's entry    (() -> TOP) -> TOP // TODO: add arc and argv
  world_entry = fn(world_exit, bot);
  world_entry->set_type(fn(world_exit, bot));
}

ir::Node::Ref ir::builder::kind()
{ return lookup_or_emplace(Node::mk_node<Kind>()); }

ir::Node::Ref ir::builder::type()
{ return lookup_or_emplace(Node::mk_node<Type>()); }

ir::Node::Ref ir::builder::prop()
{ return lookup_or_emplace(Node::mk_node<Prop>()); }

ir::Node::Ref ir::builder::unit()
{ return lookup_or_emplace(Node::mk_node<Unit>()); }

ir::Fn::Ref ir::builder::entry()
{ return world_entry; }

ir::Fn::Ref ir::builder::exit()
{ return world_exit; }

ir::Node::Ref ir::builder::id(symbol symb, Node::Ref type)
{
  assert(type != Node::no_ref && "Type must exist.");
  return lookup_or_emplace(Node::mk_node<Constructor>(symb, type));
}

ir::Node::Ref ir::builder::ignore() // TODO: replace nullptr with existential
{ return lookup_or_emplace(Node::mk_node<Constructor>("_", nullptr)); }

ir::Node::Ref ir::builder::param(Node::Ref type)
{ return lookup_or_emplace(Node::mk_node<Param>(type)); }

ir::Node::Ref ir::builder::lit(std::uint_fast64_t value)
{ return lookup_or_emplace(Node::mk_node<Literal>(value)); }

ir::Node::Ref ir::builder::binop(ir::BinaryKind op, Node::Ref lhs, Node::Ref rhs)
{ return lookup_or_emplace(Node::mk_node<Binary>(op, lhs, rhs)); }

ir::Node::Ref ir::builder::i(bool no_sign, Node::Ref size)
{ return lookup_or_emplace(Node::mk_node<Int>(no_sign, size)); }

ir::Fn::Ref ir::builder::fn(Node::Ref codomain, Node::Ref domain)
{
  assert(codomain != nullptr && "codomain must stay valid.");
  return static_cast<Fn::Ref>(lookup_or_emplace(Node::mk_node<Fn>(codomain, domain)));
}

ir::Fn::Ref ir::builder::fn()
{ return static_cast<Fn::Ref>(lookup_or_emplace(Node::mk_node<Fn>())); }

ir::Node::Ref ir::builder::app(Node::Ref caller, Node::Ref arg)
{
  assert(caller != nullptr && arg != nullptr && "caller and arg must stay valid.");
  return lookup_or_emplace(Node::mk_node<App>(caller, arg));
}

ir::Node::Ref ir::builder::destruct(Node::Ref of, std::vector<std::pair<Node::Ref, Node::Ref>> match_arms)
{ return lookup_or_emplace(Node::mk_node<Case>(of, match_arms)); }

ir::Node::Ref ir::builder::lookup_or_emplace(Node::Store store)
{
  store->mach_ = this;
  store->gid_ = gid++;

  if(auto it = nodes.find(store); it != nodes.end())
      return it->get(); // <- might be different pointer
  return nodes.emplace(std::move(store)).first->get();
}

void ir::builder::print(std::ostream& os)
{
  print(os, world_entry);
}

std::ostream& ir::builder::print(std::ostream& os, Node::Ref ref)
{
  NodeMap<std::string> ns;
  std::size_t param_count = 0;
  std::size_t fn_count = 0;

  std::uint_fast16_t defining = 0;

  std::queue<Node::cRef> defs_to_print;
  defs_to_print.push(ref);

  auto y = [&os, &ns, &param_count, &fn_count, &defining, &defs_to_print](Node::cRef ref) -> std::ostream&
  {
    auto inner = [&os, &ns, &param_count, &fn_count, &defining, &defs_to_print](auto&& y, Node::cRef ref) -> std::ostream&
    {
      switch(ref->kind())
      {
      case NodeKind::Kind: return os << "Kind"; break;
      case NodeKind::Type: return os << "Type"; break;
      case NodeKind::Prop: return os << "Prop"; break;
      case NodeKind::Unit: return os << "()"; break;

      case NodeKind::Ctr: {
          return os << static_cast<Constructor::cRef>(ref)->name.get_string();
        } break;

      case NodeKind::Int: {
          os << (static_cast<Int::cRef>(ref)->is_unsigned() ? "uint " : "int ");
          return y(y, static_cast<Int::cRef>(ref)->size());
        } break;

      case NodeKind::Literal: {
          return os << static_cast<Literal::cRef>(ref)->literal;
        } break;

      case NodeKind::Binary: {
          auto binop = static_cast<Binary::cRef>(ref);
          os << "(";
          y(y, binop->lhs()) << ")";

          switch(binop->op)
          {
          case ir::BinaryKind::Minus: os << " - "; break;
          case ir::BinaryKind::Plus:  os << " + "; break;
          case ir::BinaryKind::Mult:  os << " * "; break;
          }
          os << "(";
          return y(y, binop->rhs()) << ")";
        } break;

      case NodeKind::Case: {
          auto cs = static_cast<Case::cRef>(ref);
          os << "case (";
          y(y, cs->of()) << ") [ ";

          const auto& arms = cs->match_arms();
          for(auto it = arms.begin(); it != arms.end(); ++it)
          {
            y(y, it->first) << " => ";
            y(y, it->second);
            if(std::next(it) != arms.end())
              os << " | ";
          }
          return os << " ]";
        } break;

      case NodeKind::Param: {
          if(auto it = ns.find(ref); it != ns.end())
            return os << it->second;

          auto name = "p" + std::to_string(param_count++);
          ns.emplace(ref, name);

          if(ref->type() == Node::no_ref)
            return os << name;
          os << "(" << name << ") : ";
          return y(y, ref->type());
        } break;

      case NodeKind::App: {
          App::cRef app = static_cast<App::cRef>(ref);

          os << "(";
          y(y, app->caller()) << " ";
          return y(y, app->arg()) << ")";
        } break;

      case NodeKind::Fn: {
          Fn::cRef lm = static_cast<Fn::cRef>(ref);

          auto it = ns.find(lm);
          if(it != ns.end() && defining > 0)
            return os << (!!(defining & 1) ? "goto " : "") << it->second;
          auto name = it != ns.end() ? it->second : "f" + std::to_string(fn_count++);

          if(it == ns.end())
            ns.emplace(ref, name);

          if(defining > 0)
          {
            defs_to_print.push(ref);
            return os << (!!(defining & 1) ? "goto " : "") << name;
          }
          os << name << "(";

          defining += 2;
          y(y, lm->arg()) << "): \n    ";

          defining--;
          auto& ret = y(y, lm->bdy());
          defining--;
          return ret;
        } break;
      }
      assert(false && "Unhandled case.");
      return os;
    };

    return inner(inner, ref);
  };

  while(!defs_to_print.empty())
  {
    y(defs_to_print.front()) << ";\n";
    defs_to_print.pop();
  }
  return os;
}

ir::Node::Ref ir::builder::exec()
{ return exec(app(entry(), exit())); }


ir::Node::Ref find_ctor_and_collect_params(ir::Node::Ref ref, std::vector<ir::Node::Ref>& params)
{
  switch(ref->kind())
  {
    case ir::NodeKind::Ctr: return ref;
    case ir::NodeKind::App: {
      ir::App::Ref app = static_cast<ir::App::Ref>(ref);
      
      params.emplace_back(app->arg());
      return find_ctor_and_collect_params(app->caller(), params);
    } break;
  }
  // Not a constructor or an application using a constructor
  params.clear();
  return nullptr;
}

ir::Node::Ref ir::builder::exec(ir::Node::Ref ref)
{
  auto inner = [this](auto&& y, Node::Ref ref) -> ir::Node::Ref
  {
    switch(ref->kind())
    {
    case NodeKind::Kind: return ref; break;
    case NodeKind::Type: return ref; break;
    case NodeKind::Prop: return ref; break;
    case NodeKind::Unit: return ref; break;

    case NodeKind::Ctr:  return ref; break;

    case NodeKind::Int:  return ref; break;
    case NodeKind::Literal: return ref; break;

    case NodeKind::Binary: {
      Binary::Ref br = static_cast<Binary::Ref>(ref);

      auto l = exec(br->lhs());
      auto r = exec(br->rhs());

      if(l->kind() != NodeKind::Literal || r->kind() != NodeKind::Literal)
        return this->binop(br->op, l, r);

      auto ll = static_cast<Literal::Ref>(l);
      auto lr = static_cast<Literal::Ref>(r);
      switch (br->op)
      {
      case BinaryKind::Minus: return this->lit(ll->literal - lr->literal);
      case BinaryKind::Plus:  return this->lit(ll->literal + lr->literal);
      case BinaryKind::Mult:  return this->lit(ll->literal * lr->literal);
      }
    } break;

    case NodeKind::Case: {
        Case::Ref cs = static_cast<Case::Ref>(ref);

        auto of_v = exec(cs->of());

        if(of_v->kind() != NodeKind::Literal)
        {
          std::vector<Node::Ref> of_params;
          auto of_ctor = find_ctor_and_collect_params(of_v, of_params);

          for(auto& match : cs->match_arms())
          {
            std::vector<Node::Ref> arm_params;
            auto arm_ctor = find_ctor_and_collect_params(match.first, arm_params);
            assert(arm_ctor != nullptr && "Match arms must use constructors.");

            if(arm_ctor == of_ctor)
            {
              assert(of_params.size() == arm_params.size() && "Constructors must have the same number of arguments.");

              Node::Ref result = match.second;
              for(std::size_t i = 0; i < of_params.size(); ++i)
                result = subst(arm_params[i], of_params[i], result);
              return result;
            }
          }
        }
        else
        {
          for(auto& match : cs->match_arms())
          {
            if(of_v == match.first || (match.first->kind() == NodeKind::Ctr && static_cast<Constructor::Ref>(match.first)->name == symbol("_")))
            {
              return match.second; // if literal matches exactly or pattern is ignore, we don't have to subst anything
            }
            else if(match.first->kind() == NodeKind::Param) // want to bind this under new name
            {
              return subst(match.first, of_v, match.second);
            }
          }
        }
        // no constructor chosen             // TODO: Add some special casing for BOTTOM
        return nullptr;
      } break;

    case NodeKind::Param: return ref; break;

    case NodeKind::App: {
        App::Ref app = static_cast<App::Ref>(ref);

        auto f = y(y, app->caller());
        auto v = y(y, app->arg());

        if(f->kind() == NodeKind::Ctr)
        {
          if(static_cast<Constructor::Ref>(f)->name.get_hash() == symbol("print").get_hash())
          {
            print(std::cout, v);
            return unit();
          }
          return this->app(f, v);
        }
        assert(f->kind() == NodeKind::Fn && "Callable must be a function.");
        return this->subst(static_cast<Fn::Ref>(f)->arg(), v, static_cast<Fn::Ref>(f)->bdy());
      } break;

    case NodeKind::Fn: {
        // functions don't compute anything
        return ref;
      } break;
    }
    assert(false && "Unhandled case.");
    return nullptr;
  };
  auto res = inner(inner, ref);

  Node::Ref tmp = res;
  while(res != (tmp = inner(inner, res)))
  {
    res = tmp;
  }
  return res;
}


ir::Node::Ref ir::builder::subst(ir::Node::Ref what, ir::Node::Ref for_, ir::Node::Ref in)
{
  tsl::robin_set<Node::Ref> seen;
  return subst(what, for_, in, seen);
}

ir::Node::Ref ir::builder::subst(ir::Node::Ref what, ir::Node::Ref for_, ir::Node::Ref in, tsl::robin_set<Node::Ref>& seen)
{
  if(what == for_)
    return in; // no-op

  ir::Node::Ref ret = ir::Node::no_ref;
  if(what == in)
    ret = for_;
  else
  {
    switch(in->kind())
    {
    case NodeKind::Kind:
    case NodeKind::Type:
    case NodeKind::Prop:
    case NodeKind::Unit:
    case NodeKind::Param:
    case NodeKind::Ctr:
    case NodeKind::Int:
    case NodeKind::Literal:
      ret = in; break;

    case NodeKind::Fn: {
      Fn::Ref fn = static_cast<Fn::Ref>(in);
      if(fn->arg()->type() != nullptr)
        fn->arg()->set_type(subst(what, for_, fn->arg()->type(), seen));

      bool contains = seen.contains(in);
      seen.insert(in);

      // TODO: Fix pointer of nominals in case of recursion. perhaps we need a function that checks if fn is free in fn->bdy()
      auto b = contains ? fn->bdy() : subst(what, for_, fn->bdy(), seen);

      ret = this->fn(fn->arg(), b);
    } break;

    case NodeKind::Binary: {
      Binary::Ref bin = static_cast<Binary::Ref>(in);

      auto a = subst(what, for_, bin->lhs(), seen);
      auto b = subst(what, for_, bin->rhs(), seen);

      ret = this->binop(bin->op, a, b);
    } break;
    case NodeKind::App: {
      App::Ref app = static_cast<App::Ref>(in);

      auto a = subst(what, for_, app->caller(), seen);
      auto b = subst(what, for_, app->arg(), seen);

      ret = this->app(a, b);
    } break;

    case NodeKind::Case: {
      Case::Ref cs = static_cast<Case::Ref>(in);

      auto of = subst(what, for_, cs->of(), seen);
      auto arms = cs->match_arms();
      for(auto& match : arms)
      {
        if(match.first->type() != nullptr)
          match.first->set_type(subst(what, for_, match.first->type(), seen));
        match.second = subst(what, for_, match.second, seen);
      }
      ret = this->destruct(of, arms);
    } break;
    }
  }
  if(in->type() != nullptr)
    ret->set_type(subst(what, for_, in->type(), seen));

  return ret;
}


