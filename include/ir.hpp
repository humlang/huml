#pragma once

#include <per_statement_ir.hpp>

struct hx_ir
{
  hx_ir(std::vector<hx_per_statement_ir>&& nodes); 
private:
  std::vector<hx_per_statement_ir> nodes;
};

