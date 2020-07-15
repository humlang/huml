#include <ast.hpp>
#include "exist.hpp" //<- stored under src/

#include <type_checking.hpp>
#include <reader.hpp>
#include <cogen.hpp>

#include <backend/builder.hpp>
#include <backend/cogen.hpp>
#include <backend/sc.hpp>

#include <iterator>
#include <iostream>
#include <queue>

hx_ast::hx_ast()
{
}

void hx_ast::print(std::ostream& os, ast_ptr node)
{
  if(node == nullptr)
  {
    os << "NULL";
    return;
  }
  if(node->annot != nullptr)
    os << "((";
  switch(node->kind)
  {
  case ASTNodeKind::exist: {
      exist::ptr ex = std::static_pointer_cast<exist>(node);

      if(ex->is_solved())
        print(os, ex->solution);

      os << ex->symb.get_string();
    } break;
  case ASTNodeKind::Kind: os << "Kind"; break;
  case ASTNodeKind::Type: os << "Type"; break;
  case ASTNodeKind::Prop: os << "Prop"; break;
  case ASTNodeKind::trait_type: os << "Trait"; break;
  case ASTNodeKind::unit: os << "()"; break;
  case ASTNodeKind::number: os << std::static_pointer_cast<number>(node)->symb.get_string(); break;
  case ASTNodeKind::ptr: os << "*"; print(os, std::static_pointer_cast<pointer>(node)->of); break;
  case ASTNodeKind::assign: {
      assign::ptr as = std::static_pointer_cast<assign>(node);
      print(os, as->identifier);
      os << " := ";
      print(os, as->definition);
      os << " ; ";
      print(os, as->in);
    } break;
  case ASTNodeKind::assign_data: {
      assign_data::ptr as = std::static_pointer_cast<assign_data>(node);
      os << "data ";
      print(os, as->lhs);
      os << " : ";
      print(os, as->rhs);
      os << ";";
    } break;
  case ASTNodeKind::assign_type: {
      assign_type::ptr as = std::static_pointer_cast<assign_type>(node);
      os << "type ";
      print(os, as->lhs);
      os << " : ";
      print(os, as->rhs);
      os << ";";
    } break;
  case ASTNodeKind::trait: {
      trait::ptr t = std::static_pointer_cast<trait>(node);

      os << "trait ";
      print(os, t->name);
      os << " ";
      for(auto& x : t->params)
      {
        os << "(";
        print(os, x);
        os << " : ";
        print(os, x->annot);
        os << ") ";
      }
      os << "{\n";
      for(auto& x : t->methods)
      {
        os << "  ";
        print(os, x);
        os << ";\n";
      }
      os << "}";
    } break;
  case ASTNodeKind::implement: {
      implement::ptr imp = std::static_pointer_cast<implement>(node);

      os << "implement ";
      print(os, imp->trait);
      os << " {\n";
      for(auto& x : imp->methods)
      {
        os << "  ";
        print(os, x);
        os << ";\n";
      }
      os << "}";
    } break;
  case ASTNodeKind::expr_stmt:   {
      expr_stmt::ptr ex = std::static_pointer_cast<expr_stmt>(node);
      print(os, ex->lhs);
      os << ";";
    } break;
  case ASTNodeKind::directive: {
      directive::ptr dir = std::static_pointer_cast<directive>(node);

      if(dir->implicit_typing)
        os << "#Implicit Type;";
      else
        os << "#Explicit Type;";
    } break;
  case ASTNodeKind::identifier:  {
      identifier::ptr id = std::static_pointer_cast<identifier>(node);
      os << id->symb.get_string();
    } break;
  case ASTNodeKind::lambda:      {
      lambda::ptr lam = std::static_pointer_cast<lambda>(node);

      if(lam->name != "")
      {
        os << lam->name.get_string() << " ";

        auto uncurried = lam->uncurry();
        for(auto& a : uncurried.first)
        {
          os << "(";
          print(os, a);
          os << " : ";
          print(os, a->annot);
          os << ") ";
        }
        os << ":= ";
        print(os, uncurried.second);
      }
      else
      {
        if(lam->lhs->annot != nullptr && !hx_ast::used(lam->lhs, lam->rhs))
        {
          os << "(";
          print(os, lam->lhs->annot);
          os << " -> ";
          print(os, lam->rhs);
          os << ")";
        }
        else
        {
          os << "\\";
          print(os, lam->lhs);
          os << ". ";
          print(os, lam->rhs);
        }
      }
    } break;
  case ASTNodeKind::app:         {
      app::ptr ap = std::static_pointer_cast<app>(node);
      os << "((";
      print(os, ap->lhs);
      os << ") (";
      print(os, ap->rhs);
      os << "))";
    } break;                                  
  case ASTNodeKind::match: {
      match::ptr mm = std::static_pointer_cast<match>(node);
      
      print(os, mm->pat);
      os << " => ";
      print(os, mm->exp);
    } break;
  case ASTNodeKind::pattern_matcher: {
      pattern_matcher::ptr pm = std::static_pointer_cast<pattern_matcher>(node);

      os << "case (";
      print(os, pm->to_match);
      os << ") [";
      for(auto it = pm->data.begin(); it != pm->data.end(); ++it)
      {
        print(os, *it);
        if(std::next(it) != pm->data.end())
          os << " | ";
      }
    } break;
  }
  if(node->annot != nullptr)
  {
    os << ") : (";
    print(os, node->annot);
    os << "))";
  }
}

std::pair<std::vector<ast_ptr>, ast_ptr> lambda::uncurry() const
{
  std::vector<ast_ptr> args;
  args.emplace_back(lhs);

  ast_ptr lam = rhs;
  while(lam->kind == ASTNodeKind::lambda)
  {
    args.emplace_back(std::static_pointer_cast<lambda>(lam)->lhs);
    lam = std::static_pointer_cast<lambda>(lam)->rhs;
  }
  return { args, lam };
}

void hx_ast::print(std::ostream& os) const
{
  for(auto& root : data)
  {
    print(os, root);
    os << "\n";
  }
}

bool hx_ast::used(ast_ptr what, ast_ptr in, tsl::robin_set<identifier::ptr>& binders, bool ign_type)
{
  bool ret = false;
  switch(in->kind)
  {
  case ASTNodeKind::Kind: ret = what->kind == ASTNodeKind::Kind; break;
  case ASTNodeKind::Type: ret = what->kind == ASTNodeKind::Type; break;
  case ASTNodeKind::Prop: ret = what->kind == ASTNodeKind::Prop; break;
  case ASTNodeKind::trait_type: ret = what->kind == ASTNodeKind::trait_type; break;
  case ASTNodeKind::unit: ret = what->kind == ASTNodeKind::unit; break;
  case ASTNodeKind::number: ret = what->kind == ASTNodeKind::number
                            && std::static_pointer_cast<number>(in)->symb == std::static_pointer_cast<number>(what)->symb; break;
  case ASTNodeKind::ptr: ret = what->kind == ASTNodeKind::ptr &&
                                 used(std::static_pointer_cast<pointer>(what)->of,
                                      std::static_pointer_cast<pointer>(in)->of,
                                      binders, ign_type); break;
  case ASTNodeKind::trait: {
      trait::ptr tr = std::static_pointer_cast<trait>(in);

      // Note: We don't need to add the binder, because traits may not reference themselves.   TODO: yet?

      for(auto& x : tr->methods)
      {
        if(used(what, x, binders, ign_type))
        {
          ret = true;
          break;
        }
      }
      for(auto& x : tr->params)
      {
        if(used(what, x, binders, ign_type))
        {
          ret = true;
          break;
        }
      }
    } break;
  case ASTNodeKind::implement: {
      implement::ptr im = std::static_pointer_cast<implement>(in);

      if(used(what, im->trait, binders, ign_type))
      {
        ret = true;
      }
      else
      {
        for(auto& x : im->methods)
        {
          if(used(what, x, binders, ign_type))
          {
            ret = true;
            break;
          }
        }
      }
    } break;
  case ASTNodeKind::assign: {
      assign::ptr as = std::static_pointer_cast<assign>(in);

      if(used(what, as->definition, binders, ign_type))
        ret = true;
      else if(used(what, as->in, binders, ign_type))
        ret = true;
    } break;
  case ASTNodeKind::assign_data: {
      assign_data::ptr as = std::static_pointer_cast<assign_data>(in);

      if(used(what, as->rhs, binders, ign_type))
        ret = true;
      auto itp = binders.insert(std::static_pointer_cast<identifier>(as->lhs));
      assert(itp.second && "Binder already inserted before.");
    } break;
  case ASTNodeKind::assign_type: {
      assign_type::ptr as = std::static_pointer_cast<assign_type>(in);

      auto itp = binders.insert(std::static_pointer_cast<identifier>(as->lhs));
      assert(itp.second && "Binder already inserted before.");
      if(used(what, as->rhs, binders, ign_type))
        ret = true;
    } break;
  case ASTNodeKind::expr_stmt:   {
      expr_stmt::ptr ex = std::static_pointer_cast<expr_stmt>(in);

      if(used(what, ex->lhs, binders, ign_type))
        ret = true;
    } break;
  case ASTNodeKind::directive:   {
      ret = false;
    } break;
  case ASTNodeKind::identifier:  {
      identifier::ptr id = std::static_pointer_cast<identifier>(in);

      if(id->symb == symbol("_"))
        return false;  // always just ignore the underscore

      if(what->kind != ASTNodeKind::identifier)
        ret = false;
      else
      {
        identifier::ptr whatid = std::static_pointer_cast<identifier>(what);

        // Variable is only used if the context hasn't seen its binder before
        ret = whatid->symb == id->symb && !binders.contains(std::static_pointer_cast<identifier>(id));
      }
    } break;
  case ASTNodeKind::lambda:      {
      lambda::ptr lam = std::static_pointer_cast<lambda>(in);
      assert(lam->lhs->kind == ASTNodeKind::identifier && "Bug in parser.");

      auto itp = binders.insert(std::static_pointer_cast<identifier>(lam->lhs));
      assert(itp.second && "Insertion must succeed, we can't have \"\\x. \\x. x\" where all x are the same thing");
      
      if(used(what, lam->rhs, binders, ign_type))
        ret = true;
      else
      {
        assert(!binders.empty());
        binders.erase(binders.find(std::static_pointer_cast<identifier>(lam->lhs)));

        if(lam->lhs->annot)
          ret = used(what, lam->lhs->annot, binders, ign_type);
      }
    } break;
  case ASTNodeKind::app:         {
      app::ptr ap = std::static_pointer_cast<app>(in);

      if(used(what, ap->lhs, binders, ign_type))
        ret = true;
      else if(used(what, ap->rhs, binders, ign_type))
        ret = true;
    } break;                                  
  case ASTNodeKind::pattern_matcher: {
      pattern_matcher::ptr pm = std::static_pointer_cast<pattern_matcher>(in);

      if(used(what, pm->to_match))
        ret = true;
      else
      {
        for(auto& p : pm->data)
        {
          if(used(what, p))
          {
            ret = true;
            break;
          }
        }
      }
    } break;
  case ASTNodeKind::match: {
      match::ptr mm = std::static_pointer_cast<match>(in);

      // TODO: FIX. This messes up things, since to_match might bind identifiers
      if(used(what, mm->pat))
        ret = true;
      else
        ret = used(what, mm->exp);
    } break;
  }
  return ret || (in->annot && !ign_type ? used(what, in->annot, binders, ign_type) : false);
}

void hx_ast::print_as_type(std::ostream& os, ast_ptr node)
{
  if(node == nullptr)
  {
    os << "NULL";
    return;
  }
  if(node->type != nullptr)
    os << "((";
  switch(node->kind)
  {
  case ASTNodeKind::exist: {
      exist::ptr ex = std::static_pointer_cast<exist>(node);

      if(ex->is_solved())
        print(os, ex->solution);

      os << ex->symb.get_string();
    } break;
  case ASTNodeKind::Kind: os << "Kind"; break;
  case ASTNodeKind::Type: os << "Type"; break;
  case ASTNodeKind::Prop: os << "Prop"; break;
  case ASTNodeKind::trait_type: os << "Trait"; break;
  case ASTNodeKind::unit: os << "()"; break;
  case ASTNodeKind::number: os << std::static_pointer_cast<number>(node)->symb.get_string(); break;
  case ASTNodeKind::ptr: os << "*"; print(os, std::static_pointer_cast<pointer>(node)->of); break;

  case ASTNodeKind::assign_data:
  case ASTNodeKind::assign_type:
  case ASTNodeKind::expr_stmt:
  case ASTNodeKind::directive:
  case ASTNodeKind::implement:
  case ASTNodeKind::assign:
  case ASTNodeKind::trait:
                          assert(false && "Statements are no types."); break;

  case ASTNodeKind::identifier:  {
      identifier::ptr id = std::static_pointer_cast<identifier>(node);
      os << id->symb.get_string();
    } break;
  case ASTNodeKind::lambda:      {
      lambda::ptr lam = std::static_pointer_cast<lambda>(node);

      assert(lam->name != symbol("") && "must not be external");
      if(lam->lhs->type != nullptr && !hx_ast::used(lam->lhs, lam->rhs))
      {
        os << "(";
        print_as_type(os, lam->lhs->type);
        os << " -> ";
        print_as_type(os, lam->rhs);
        os << ")";
      }
      else
      {
        os << "\\";
        print_as_type(os, lam->lhs);
        os << ". ";
        print_as_type(os, lam->rhs);
      }
    } break;
  case ASTNodeKind::app:         {
      app::ptr ap = std::static_pointer_cast<app>(node);
      os << "((";
      print_as_type(os, ap->lhs);
      os << ") (";
      print_as_type(os, ap->rhs);
      os << "))";
    } break;                                  
  case ASTNodeKind::match: {
      match::ptr mm = std::static_pointer_cast<match>(node);
      
      print_as_type(os, mm->pat);
      os << " => ";
      print_as_type(os, mm->exp);
    } break;
  case ASTNodeKind::pattern_matcher: {
      pattern_matcher::ptr pm = std::static_pointer_cast<pattern_matcher>(node);

      os << "case (";
      print_as_type(os, pm->to_match);
      os << ") [";
      for(auto it = pm->data.begin(); it != pm->data.end(); ++it)
      {
        print_as_type(os, *it);
        if(std::next(it) != pm->data.end())
          os << " | ";
      }
    } break;
  }
  if(node->type != nullptr)
  {
    os << ") : (";
    print_as_type(os, node->type);
    os << "))";
  }
}

ast_ptr handle_id(ast_ptr potential_id, scope_base& sc, ASTNodePtrCache& seen, ScopingIndices& child_indices, hx_ast* self)
{
  if(!potential_id)
    return potential_id;
  else if(potential_id->kind != ASTNodeKind::identifier)
  {
    self->consider_scoping(sc, seen, child_indices, potential_id);
    return potential_id;
  }
  auto id = std::static_pointer_cast<identifier>(potential_id);
  
  if(id->is_binding)
    return id;

  // We need to substitute this identifier with its binding
  auto symb_binding = sc.get(id->symb);
  
  // TODO: emit diagnostic instead of assert
  assert(symb_binding != nullptr && "binding for unbound identifier must exist");

  return symb_binding;
}

void hx_ast::consider_scoping(scope_base& sc, ASTNodePtrCache& seen, ScopingIndices& child_indices, ast_ptr p)
{
  switch(p->kind)
  {
    default: assert(false && "Unconsidered case"); break;

    case ASTNodeKind::Kind:
    case ASTNodeKind::Type:
    case ASTNodeKind::unit:
    case ASTNodeKind::Prop:
    case ASTNodeKind::number:
    case ASTNodeKind::trait_type:
      break;

    case ASTNodeKind::trait: {
      auto x = std::static_pointer_cast<trait>(p);

      auto* lower = sc.children[child_indices[&sc]++].get();
      for(auto& v : x->params)
      {
        v = handle_id(v->annot, *lower, seen, child_indices, this);

        lower = lower->children[child_indices[lower]++].get();
      }
      for(auto& v : x->methods)
        v = handle_id(v, *lower, seen, child_indices, this);
    } break;
    case ASTNodeKind::implement: {
      auto x = std::static_pointer_cast<implement>(p);

      x->trait = handle_id(x->trait, sc, seen, child_indices, this);
      for(auto& v : x->methods)
        v = handle_id(v, sc, seen, child_indices, this);
    } break;
    case ASTNodeKind::ptr: {
      auto x = std::static_pointer_cast<pointer>(p);

      x->of = handle_id(x->of, sc, seen, child_indices, this);
    } break;
    case ASTNodeKind::app: {
      auto x = std::static_pointer_cast<app>(p);

      x->lhs = handle_id(x->lhs, sc, seen, child_indices, this);
      x->rhs = handle_id(x->rhs, sc, seen, child_indices, this);
    } break;
    case ASTNodeKind::lambda: {
      auto x = std::static_pointer_cast<lambda>(p);

      x->lhs->annot = handle_id(x->lhs->annot, sc, seen, child_indices, this);
      x->rhs = handle_id(x->rhs, *sc.children[child_indices[&sc]++], seen, child_indices, this);
    } break;
    case ASTNodeKind::match: {
      auto x = std::static_pointer_cast<match>(p);

      auto lower = *sc.children[child_indices[&sc]++];
      x->pat = handle_id(x->pat, lower, seen, child_indices, this);
      x->exp = handle_id(x->exp, lower, seen, child_indices, this);
    } break;
    case ASTNodeKind::pattern_matcher: {
      auto x = std::static_pointer_cast<pattern_matcher>(p);

      x->to_match = handle_id(x->to_match, sc, seen, child_indices, this);

      for(auto& a : x->data)
        a = handle_id(a, sc, seen, child_indices, this);
    } break;
    case ASTNodeKind::assign: {
      auto x = std::static_pointer_cast<assign>(p);

      x->definition = handle_id(x->definition, sc, seen, child_indices, this);
      x->in = handle_id(x->in, *sc.children[child_indices[&sc]++], seen, child_indices, this);
    } break;
    case ASTNodeKind::assign_type: {
      auto x = std::static_pointer_cast<assign_type>(p);

      x->rhs = handle_id(x->rhs, sc, seen, child_indices, this);
    } break;
    case ASTNodeKind::assign_data: {
      auto x = std::static_pointer_cast<assign_data>(p);

      x->rhs = handle_id(x->rhs, sc, seen, child_indices, this);
    } break;
    case ASTNodeKind::expr_stmt: {
      auto x = std::static_pointer_cast<expr_stmt>(p);

      x->lhs = handle_id(x->lhs, sc, seen, child_indices, this);
    } break;
  }
  if(p->annot && !seen.count(p))
    consider_scoping(sc, seen, child_indices, p->annot);
}

void hx_ast::consider_scoping(scoping_context& ctx)
{
  // visit all identifiers, see if they need fixing
  ASTNodePtrCache seen;
  ScopingIndices child_indices;
  std::queue<ast_ptr> to_visit;
  for(auto& p : data)
    consider_scoping(ctx.base, seen, child_indices, p);
}

bool hx_ast::type_checks(scoping_context& ctx)
{
  hx_ast_type_checking checker;
  bool type_checks = true;

  typing_context tctx;
  //// Add basic defs
  /// __reinterpret        ~        \(A : Type). \(a : A). \(B : Type). B
  auto reinterpret_id = std::make_shared<identifier>(symbol("__reinterpret"));
  {
  auto A = std::make_shared<identifier>(symbol("A")); A->annot = A->type = std::make_shared<type>();
  auto a = std::make_shared<identifier>(symbol("a")); a->annot = a->type = A;
  auto B = std::make_shared<identifier>(symbol("B")); B->annot = B->type = std::make_shared<type>();
  auto reinterpret_type = std::make_shared<lambda>(A, std::make_shared<lambda>(a, std::make_shared<lambda>(B, B)));
  tctx.data.emplace_back(reinterpret_id, reinterpret_type);
  }
  ctx.base.bindings.emplace(symbol("__reinterpret"), reinterpret_id);
      // TODO: do we need to add stuff to `ctx.cur_scope->bindings`?
  /// nat
  auto nat_id = std::make_shared<identifier>(symbol("nat"));
  auto nat_type = std::make_shared<type>();
  auto nat_type_assign = std::make_shared<assign_type>(nat_id, nat_type);

  ctx.base.bindings.emplace(symbol("nat"), nat_id);
  tctx.data.emplace_back(nat_id, nat_type);
  /// bytes
  auto bytes_id   = std::make_shared<identifier>(symbol("bytes"));

  auto bytes_underscore = std::make_shared<identifier>(symbol("_"));
  bytes_underscore->annot = nat_id;
  auto bytes_type = std::make_shared<lambda>(bytes_underscore, std::make_shared<type>());
  auto bytes_type_assign = std::make_shared<assign_type>(bytes_id, bytes_type);

  ctx.base.bindings.emplace(symbol("bytes"), bytes_id);
  tctx.data.emplace_back(bytes_id, bytes_type);
  /// __nat_to_bytes_signed   ~     \(A : Type). \(n : Nat). \(a : A). bytes n
  auto nat_to_bytes_signed_id = std::make_shared<identifier>(symbol("__nat_to_bytes_signed"));
  {
  auto A = std::make_shared<identifier>(symbol("A")); A->annot = A->type = std::make_shared<type>();
  auto n = std::make_shared<identifier>(symbol("n")); n->annot = n->type = nat_id;
  auto a = std::make_shared<identifier>(symbol("a")); a->annot = a->type = A;
  auto bytes_n = std::make_shared<app>(bytes_id, n);
  tctx.data.emplace_back(nat_to_bytes_signed_id, std::make_shared<lambda>(A,
                                                    std::make_shared<lambda>(n,
                                                      std::make_shared<lambda>(a, bytes_n))));
  }
  ctx.base.bindings.emplace(symbol("__nat_to_bytes_signed"), nat_to_bytes_signed_id);
  /// __nat_to_bytes_unsigned   ~     \(A : Type). \(n : Nat). \(a : A). bytes n
  auto nat_to_bytes_unsigned_id = std::make_shared<identifier>(symbol("__nat_to_bytes_unsigned"));
  {
  auto A = std::make_shared<identifier>(symbol("A")); A->annot = A->type = std::make_shared<type>();
  auto n = std::make_shared<identifier>(symbol("n")); n->annot = n->type = nat_id;
  auto a = std::make_shared<identifier>(symbol("a")); a->annot = a->type = A;
  auto bytes_n = std::make_shared<app>(bytes_id, n);
  tctx.data.emplace_back(nat_to_bytes_unsigned_id, std::make_shared<lambda>(A,
                                                    std::make_shared<lambda>(n,
                                                      std::make_shared<lambda>(a, bytes_n))));
  }
  ctx.base.bindings.emplace(symbol("__nat_to_bytes_unsigned"), nat_to_bytes_unsigned_id);
  //// fixup
  consider_scoping(ctx);

  /// typecheck
  for(auto& root : data)
  {
    auto typ = checker.find_type(tctx, root);

    if(typ == nullptr)
      type_checks = false;
  }
  return type_checks;
}

void hx_ast::cogen(std::string output_file) const
{
  ir::builder mach;

  std::vector<ir::Node::cRef> nodes;
  for(auto& x : data)
    nodes.emplace_back(::cogen(mach, x));

  auto entry = mach.root(nodes);

  ir::supercompile(mach, entry);
  ir::cogen(output_file, entry);
}


