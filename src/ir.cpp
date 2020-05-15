#include <ir.hpp>
#include <types.hpp>

#include <iterator>

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
  std::uint_fast32_t node_index = 0;
  for(auto& n : this->nodes)
  {
    for(auto fv_p_it = n.free_variable_roots.begin();
        fv_p_it != n.free_variable_roots.end();
        ++fv_p_it)
    {
      auto& fv_p = *fv_p_it;

      /// 2. Only print if it is not a data constructor
      if(!data_constructors.contains(fv_p.first) && fv_p.second.potentially_bounded_ref == static_cast<std::uint_fast32_t>(-1))
      {
        // Check if we know this IR       TODO: Improve performance by not doing linear searches!
        auto fit = std::find_if(this->nodes.begin(), this->nodes.end(), [&fv_p](hx_per_statement_ir& hir)
            {
              if(!hir.node_name)   // <- may happen for expression statements
                return false;
              return (!hir.node_name->get_string().empty() && *hir.node_name == fv_p.first);
            });

        if(fit != this->nodes.end())
        {
          edges.push_back(edge { node_index, static_cast<std::uint_fast32_t>(fit - this->nodes.begin()) });
        }
        else
          globally_free_variables.push_back(fv_p.first);
        // TODO: make sure this thing is unique for the symbol in fv_p
      }
    }
    ++node_index;
  }
}

