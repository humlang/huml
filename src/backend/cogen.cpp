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

  //////////////////////////////// TYPES
  auto genty = [&gen](auto genty, Node::cRef node) -> gccjit::type
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
          // Int, Char
          case hash_string("u"):
          case hash_string("i"): return cogen_int(gen, c, ap->arg());

          // Pointer
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
  std::optional<gccjit::block> cur_block;
  NodeMap<gccjit::rvalue> rvals;
  NodeMap<gccjit::rvalue> lvals;

  auto cogen = [&gen,&fns,&lvals,&rvals,&cur_block,&genty](auto cogen, const Node* node) -> void
  {
    switch(node->kind())
    {
    case NodeKind::Fn: {
      Fn::cRef fn = node->to<Fn>();
      // check if function already exists and can be used for calling
      if(auto it = fns.find(fn); it != fns.end())
        return ;

      if(fn->is_external())
      {
        // generate ordinary function
        std::vector<gccjit::param> params;
        if(fn->arg()->kind() == NodeKind::Tup)
        {
          // If we have a function such as `main(argc : int, argv : const char**)` we generate more than one param
          Tup::cRef tup = fn->arg()->to<Tup>();
          for(std::size_t i = 0; i < tup->argc(); ++i)
          {
            auto par = gen.ctx.new_param(genty(genty, tup->me()[i]), tup->me()[i]->unique_name().get_string().c_str());

            rvals[tup->me()[i]] = par;
            params.push_back(par);
          }
        }
        else
        {
          auto par = gen.ctx.new_param(genty(genty, fn->arg()), fn->arg()->unique_name().get_string().c_str());
          rvals[fn->arg()] = par; // <- one should not be able to change parameter values in an external function, thus the missing lvals insertion

          params.push_back(par);
        }
        auto jit_fn = gen.ctx.new_function(fn->is_external() ? GCC_JIT_FUNCTION_EXPORTED : GCC_JIT_FUNCTION_IMPORTED,
                                           gen.ctx.get_type(GCC_JIT_TYPE_VOID),
                                           (fn->is_external() ? fn->external_name() : fn->unique_name()).get_string(),
                                           params, 0);
        cur_block = jit_fn.new_block(fn->unique_name().get_string().c_str());
        fns[fn] = { jit_fn, *cur_block };
        // add call to other function or case analysis
        cogen(cogen, fn->bdy());
      }
      else
      {
        assert(cur_block != std::nullopt && "Cannot start in the middle of nowhere.");

        // Generate basic block within a function
        if(fn->arg()->kind() == NodeKind::Tup)
        {
          // If we have a function such as `foo(n : int, m : int)` we generate more than one local
          auto tup = *fn->arg()->to<Tup>();
          for(std::size_t i = 0; i < tup.argc(); ++i)
          {
            lvals[tup[i]] = rvals[tup[i]] = cur_block->get_function()
                            .new_local(genty(genty, tup[i]),
                                       tup[i]->unique_name().get_string().c_str());
          }
        }
        else
        {
          rvals[fn->arg()] = lvals[fn->arg()] = cur_block->get_function()
            .new_local(genty(genty, fn->arg()),
                       fn->arg()->unique_name().get_string().c_str());
        }
        auto new_block = cur_block->get_function().new_block(fn->unique_name().get_string().c_str());

        // go from old block to new block. This is the same as a curried function call
        cur_block->end_with_jump(new_block);

        cur_block = new_block;
      }
      return ;
    } break;

    case NodeKind::App: {
      // TODO: !!!!!!!!!!!!!
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
      // results are stored in rvals by now
      assert(rvals.contains(bn->lhs()) && rvals.contains(bn->rhs()) && "Bug in codegen.");

      auto l = rvals[bn->lhs()];
      auto r = rvals[bn->rhs()];
      switch(bn->op)
      {
      case BinaryKind::Minus: rvals[bn] = l - r; break;
      case BinaryKind::Plus:  rvals[bn] = l + r; break;
      case BinaryKind::Mult:  rvals[bn] = l * r; break;
      }
      return ;
    } break;
    }
    assert(false && "Unsupported node for this backend.");
  };
  cogen(cogen, ref);

  // TODO: remove this, this hooks up the bb from main to return 42
  cur_block->end_with_return(gen.ctx.new_rvalue(gen.ctx.get_type(GCC_JIT_TYPE_INT), 42));
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


