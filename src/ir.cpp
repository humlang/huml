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

      std::cout << fv_p.first.get_string();

      if(std::next(fv_p_it) != n.free_variable_roots.end())
        std::cout << ", ";
    }

    std::cout << " };\n\n";
  }
}

