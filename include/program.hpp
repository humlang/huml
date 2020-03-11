#pragma once

#include <vm_symbol_table.hpp>
#include <vm_opcodes.hpp>
#include <token.hpp>
#include <vector>

namespace ass
{
  struct program
  {
    using symbol_table_t = std::unordered_map<std::uint_fast32_t,
                                              std::pair<symbol_type, std::size_t>>;
    friend struct assembler;

    program(const std::vector<ass::instruction>& instr) : instructions(instr)
    {  }

    program() : instructions()
    {  }

    std::vector<unsigned char> to_u8_vec() const;

    const std::pair<symbol_type, std::size_t>& lookup_symbol(const symbol& name) const;
  private:
    std::vector<ass::instruction> instructions;
    std::vector<unsigned char> bytes;
    symbol_table_t symbol_table;
  };

}

