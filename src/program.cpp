#include <program.hpp>

#include <algorithm>

namespace ass
{

std::vector<unsigned char> program::to_u8_vec() const
{
  std::vector<unsigned char> v;
  v.reserve(instructions.size() * 3);

  for(auto& instr : instructions)
  {
    const auto& intermediate = instr.to_u8_vec();

    std::move(intermediate.begin(), intermediate.end(), std::back_inserter(v));
  }

  v.shrink_to_fit();
  return v;
}

}
