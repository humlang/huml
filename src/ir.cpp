#include <ir.hpp>

#include <iostream>

hx_ir::hx_ir(std::vector<hx_per_statement_ir>&& nodes)
  : nodes(std::move(nodes))
{
  // create edges

  /// 1. Collect all free variables of the type declarations.
  symbol_set data_constructors;
  for(auto& n : this->nodes)
  {
    assert(!n.kinds.empty() && "IR cannot contain zero sized node, empty modules are disallowed!");

    if(n.kinds.back() == IRNodeKind::assign_type)
    {
      assert(n.types.size() == 1 && "Assign type must yield exactly one type.");
      assert(n.types.back()->is_inductive() && "Assign type must be inductive.");
      
      auto& ityp = *static_cast<inductive*>(n.types.back().get());
      for(auto& c : ityp.constructors)
        data_constructors.insert(c->name());
    }
  }
  for(auto& n : this->nodes)
  {
    if(!n.node_name)
    {
      std::cout << "FV("; n.print(std::cout); std::cout << ") = { ";
    }
    else
    {
      std::cout << "FV(" << n.node_name->get_string() << ") = { ";
    }

    for(auto fv_p_it = n.free_variable_roots.begin();
        fv_p_it != n.free_variable_roots.end();
        ++fv_p_it)
    {
      auto& fv_p = *fv_p_it;

      /// 2. Only print if it is not a data constructor
      if(!data_constructors.contains(fv_p.first) && fv_p.second.potentially_bounded_ref == static_cast<std::uint_fast32_t>(-1))
      {
        std::cout << fv_p.first.get_string();

        if(std::next(fv_p_it) != n.free_variable_roots.end())
          std::cout << ", ";
      }
    }

    std::cout << " };\n\n";
  }
}

