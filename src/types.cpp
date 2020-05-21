#include <types.hpp>

type_table::type_table()
  : types()
{
  // Kind
  types.emplace_back(std::make_shared<Kind_sort>());

  // Type
  types.emplace_back(std::make_shared<Type_sort>());

  // Prop
  types.emplace_back(std::make_shared<Prop_sort>());


  // Unit    is inhabited by  \\(A : Type). \\(a : A). a
  std::shared_ptr<type_base> A = std::make_shared<IdTypeBox>("A", types[Type_sort_idx]);
  types.emplace_back(std::make_shared<pi_type>(std::static_pointer_cast<IdTypeBox>(A),
                      std::make_shared<pi_type>(std::make_shared<IdTypeBox>("_", A),
                                                A)));
}
