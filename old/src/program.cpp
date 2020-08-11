#include <program.hpp>

#include <algorithm>
#include <cassert>

namespace ass
{

std::vector<unsigned char> program::to_u8_vec() const
{
  std::vector<unsigned char> v;
  v.reserve(instructions.size() * 3);

  for(auto& instr : instructions)
  {
    const auto& intermediate = instr.to_u8_vec(*this);

    std::move(intermediate.begin(), intermediate.end(), std::back_inserter(v));
  }

  v.shrink_to_fit();
  return v;
}

const std::pair<symbol_type, std::size_t>& program::lookup_symbol(const symbol& name) const
{
  auto cit = symbol_table.find(name.get_hash());
  assert(cit != symbol_table.end());
  return cit->second;
}

}
