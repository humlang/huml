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

bool cogen(generator& gen, const Node* ref)
{
  auto cogen = [&gen](auto cogen, const Node* node) -> void*
  {
    switch(node->kind())
    {
    case NodeKind::Binary: {

      } break;
    default:
     return nullptr;
    }
  };
  return cogen(cogen, ref);
}

bool cogen(std::string name, const Node* ref)
{
  generator gen(name);
  cogen(gen, ref);


  return true;
}

}


