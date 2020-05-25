#include <types.hpp>

#include <queue>

type_table::type_table()
{
  // Kind
  type_tags::kind.make_node(*this, TypeData { "Kind" });

  // Type
  type_tags::type.make_node(*this, TypeData { "Type" });

  // Prop
  type_tags::prop.make_node(*this, TypeData { "Prop" });

  // Unit    is inhabited by  \\(A : Type). \\(a : A). a
  auto A = type_tags::id.make_node(*this, TypeData { "A", { }, Type_sort_idx });
  type_tags::pi.make_node(*this, TypeData { "1",
      { A,
      type_tags::pi.make_node(*this, TypeData { "", {
        type_tags::id.make_node(*this, TypeData { "_", { }, A }),
        A
        }}
      )} });
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

