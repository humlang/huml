#include <ir.hpp>
#include <types.hpp>
#include <type_checking.hpp>

#include <iterator>

#include <iostream>

hx_ir::hx_ir()
  : nodes()
{  }

bool hx_ir::type_checks()
{
  hx_ir_type_checking typch(types, *this);
  typing_context ctx;
  ctx.reserve(1024);

  bool success = true;
  for(std::size_t stmt = 0; stmt < this->nodes.size(); ++stmt)
  {
    auto t = typch.synthesize(ctx, stmt, 0);

    auto& n = this->nodes[stmt];
    if(t == static_cast<std::uint_fast32_t>(-1))
    {
      n.print(std::cout, this->types); std::cout << " # Does not typecheck.\n";
    }
    else
    {
      n.print(std::cout, this->types); std::cout << " # Has type "; this->types.print_type(std::cout, t);
      std::cout << "\n";
    }
    success = success && (t != static_cast<std::uint_fast32_t>(-1));
  }
  return success;
}

void hx_ir::build_graph()
{
  // create edges

  /// 1. Collect all free variables of the type declarations.
  symbol_set data_constructors;
  for(auto& ctors_for_type : types.constructors)
  {
    for(auto& n : ctors_for_type.second.data)
    {
      data_constructors.insert(n.name);
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
        {
          globally_free_variables.push_back(fv_p.first);
        }
        // TODO: make sure this thing is unique for the symbol in fv_p
      }
    }
    ++node_index;
  }
}

