#include <backend/cogen.hpp>
#include <backend/node.hpp>

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

void cogen(generator& gen, const Node* ref)
{
  NodeMap<std::pair<gccjit::function, gccjit::block>> fns;
  NodeMap<gccjit::rvalue> rvals;

  auto genty = [&gen](const Node* node) -> gccjit::type
  {
    switch(node->kind())
    {
    case NodeKind::Int: {
        Int::cRef i = node->to<Int>();

        // TODO: emit normal error?
        assert(i->size()->kind() == NodeKind::Literal && "int is expected to have a constant value as argument here.");
        Literal::cRef l = i->size()->to<Literal>();

        switch(l->literal)
        {
        case 8:  return gen.ctx.get_type(i->is_unsigned() ? GCC_JIT_TYPE_UNSIGNED_CHAR  : GCC_JIT_TYPE_CHAR);
        case 16: return gen.ctx.get_type(i->is_unsigned() ? GCC_JIT_TYPE_UNSIGNED_SHORT : GCC_JIT_TYPE_SHORT);
        case 32: return gen.ctx.get_type(i->is_unsigned() ? GCC_JIT_TYPE_UNSIGNED_INT   : GCC_JIT_TYPE_INT);
        }
        assert(false && "unsupported int size for this backend.");
        return *((gccjit::type*)nullptr);
      } break;
    }
    assert(false && "Unsupported type");
    return *((gccjit::type*)nullptr);
  };
  auto cogen = [&gen,&fns,&rvals,&genty](auto cogen, const Node* node) -> void
  {
    switch(node->kind())
    {
    case NodeKind::Fn: {
      Fn::cRef fn = node->to<Fn>();

      if(auto it = fns.find(fn); it != fns.end())
        return ;
      std::vector params = { gen.ctx.new_param(genty(fn->arg()), fn->arg()->to<Param>()->unique_name().get_string().c_str()) };
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


