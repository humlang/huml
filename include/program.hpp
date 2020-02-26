#pragma once

#include <vm_opcodes.hpp>
#include <token.hpp>
#include <vector>

namespace ass
{
  struct program
  {
    program(const std::vector<ass::instruction>& instr) : instructions(instr)
    {  }

    program() : instructions()
    {  }

    std::vector<unsigned char> to_u8_vec() const;
    
  private:
    std::vector<ass::instruction> instructions;
  };

}

