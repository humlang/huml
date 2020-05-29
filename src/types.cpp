#include <types.hpp>

#include <queue>

type_table::type_table()
{
  // Kind
  IRTags::Kind.make_node(*this, IRData { "Kind" }, IRDebugData {});

  // Type
  IRTags::Type.make_node(*this, IRData { "Type" }, IRDebugData {});

  // Prop
  IRTags::Prop.make_node(*this, IRData { "Prop" }, IRDebugData {});

  // Unit    is inhabited by  \\(A : Type). \\(_ : A). a
  auto A = IRTags::identifier.make_node(*this, IRData { "A", { }, Type_sort_idx }, IRDebugData {});
  auto underscore = IRTags::identifier.make_node(*this, IRData { "_", { }, A }, IRDebugData {});  

  auto inner = IRTags::lambda.make_node(*this, IRData { "", { underscore, A } }, IRDebugData {});

  IRTags::lambda.make_node(*this, IRData { "1", { A, inner } }, IRDebugData {});
}

// [A / alpha]C                                         C                       A                      alpha
std::uint_fast32_t type_table::subst(std::uint_fast32_t in, std::uint_fast32_t what, std::uint_fast32_t with)
{
  if(in == what)
    return with;
  for(auto& x : data[in].args)
    x = subst(x, what, with);

  return in;
}

