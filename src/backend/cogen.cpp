#include <backend/cogen.hpp>
#include <backend/node.hpp>
#include <tmp.hpp>

#include <libgccjit++.h>

namespace ir
{

struct generator
{
  generator(std::string name)
    : ctx(gccjit::context::acquire())
  {
    ctx.set_int_option(GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL, 2);
  }

  ~generator()
  {
    ctx.release();
  }

  gccjit::context ctx;
};

gccjit::type cogen_int(generator& gen, Constructor::cRef ctor, Node::cRef arg)
{
  // TODO: emit normal error?
  assert(arg->kind() == NodeKind::Literal && "int is expected to have a constant value as argument here.");
  Literal::cRef l = arg->to<Literal>();

  const bool is_unsigned = ctor->name == symbol("u");
  switch(l->literal)
  {
  case 8:  return gen.ctx.get_type(is_unsigned ? GCC_JIT_TYPE_UNSIGNED_CHAR  : GCC_JIT_TYPE_CHAR);
  case 16: return gen.ctx.get_type(is_unsigned ? GCC_JIT_TYPE_UNSIGNED_SHORT : GCC_JIT_TYPE_SHORT);
  case 32: return gen.ctx.get_type(is_unsigned ? GCC_JIT_TYPE_UNSIGNED_INT   : GCC_JIT_TYPE_INT);
  }
  assert(false && "unsupported int size for this backend.");
  return *((gccjit::type*)nullptr);
}

void cogen(generator& gen, const Node* ref)
{
  NodeMap<std::pair<gccjit::function, gccjit::block>> fns;
  NodeMap<gccjit::rvalue> rvals;
  NodeMap<gccjit::rvalue> params;

  //////////////////////////////// TYPES
  auto genty = [&gen](auto genty, const Node* node) -> gccjit::type
  {
    switch(node->kind())
    {
    case NodeKind::App: {
        App::cRef ap = node->to<App>();

        if(ap->caller()->kind() == NodeKind::Ctr)
        {
          Constructor::cRef c = node->to<Constructor>();
          switch(c->name.get_hash())
          {
          case hash_string("u"):
          case hash_string("i"): return cogen_int(gen, c, ap->arg());

          case hash_string("_Ptr"): {
              return genty(genty, ap->arg()).get_pointer();
            } break;
          }
        }
      } break;
    }
    assert(false && "Unsupported type");
    return *((gccjit::type*)nullptr);
  };
  //////////////////////////////// COGEN
  auto cogen = [&gen,&fns,&rvals,&params,&genty](auto cogen, const Node* node) -> void
  {
    switch(node->kind())
    {
    case NodeKind::Fn: {
      Fn::cRef fn = node->to<Fn>();

      if(auto it = fns.find(fn); it != fns.end())
        return ;
      auto par = gen.ctx.new_param(genty(genty, fn->arg()), fn->arg()->to<Param>()->unique_name().get_string().c_str());
      params[fn->arg()] = par;

      std::vector params = { par };
      auto jit_fn = gen.ctx.new_function(fn->is_external() ? GCC_JIT_FUNCTION_EXPORTED : GCC_JIT_FUNCTION_IMPORTED,
                                         gen.ctx.get_type(GCC_JIT_TYPE_VOID),
                                         (fn->is_external() ? fn->external_name() : fn->unique_name()).get_string(),
                                         params, 0);
      gccjit::block fn_block = jit_fn.new_block();
      fns[fn] = { jit_fn, fn_block };
      // add call to other function or case analysis
      cogen(cogen, fn->bdy());

      fn_block.end_with_return();
      return ;
    } break;
    case NodeKind::Literal: {
      rvals[node] = gen.ctx.new_rvalue(gen.ctx.get_type(GCC_JIT_TYPE_LONG_LONG),
                                       static_cast<long int>(node->to<Literal>()->literal));
      return ;
    } break;
    case NodeKind::Binary: {
      Binary::cRef bn = node->to<Binary>();

      cogen(cogen, bn->lhs());
      cogen(cogen, bn->rhs());

      // How to continue? What are our arguments??
      switch(bn->op)
      {
      case BinaryKind::Minus: break;
      case BinaryKind::Plus:  break;
      case BinaryKind::Mult:  break;
      }
    } break;
    }
    assert(false && "Unsupported node for this backend.");
  };
  cogen(cogen, ref);
}

bool cogen(std::string name, const Node* ref)
{
  generator gen(name);
  cogen(gen, ref);

  // always emit assembler for now, is most versatile
  gen.ctx.compile_to_file(GCC_JIT_OUTPUT_KIND_ASSEMBLER, (name + ".s").c_str());
  return true;
}

}


