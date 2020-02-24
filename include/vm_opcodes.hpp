#pragma once

#include <cstdint>

enum class op_code : std::int_fast8_t
{
  HALT,
  LOAD,
  ADD,
  SUB,
  MUL,
  DIV,
  JMP,
  JMPREL,
  JMP_CMP,
  JMP_NCMP,
  EQUAL,
  GREATER,
  LESS,
  GREATER_EQUAL,
  LESS_EQUAL,
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

