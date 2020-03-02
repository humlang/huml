#pragma once

#include <cstdint>

enum class op_code : std::int_fast8_t
{
  HALT           = 0,
  LOAD           = 1,
  ADD            = 2,
  SUB            = 3,
  MUL            = 4,
  DIV            = 5,
  JMP            = 6,
  JMPREL         = 7,
  JMP_CMP        = 8,
  JMP_NCMP       = 9,
  EQUAL          = 10,
  GREATER        = 11,
  LESS           = 12,
  GREATER_EQUAL  = 13,
  LESS_EQUAL     = 14,
  ALLOC          = 15,
  UNKNOWN        
};

constexpr op_code byte_to_opcode(unsigned char byte)
{ return static_cast<op_code>(byte); }

constexpr unsigned char opcode_to_byte(op_code byte)
{ return static_cast<unsigned char>(byte); }


struct instruction
{
  instruction(op_code op) : opc(op)
  {  }
  
  op_code opcode() const
  { return opc; }
private:
  op_code opc;
};

