#include <vm.hpp>

#include <iostream>
#include <iomanip>
#include <cassert>

vm::vm(std::size_t initial_heap_size)
  : regs( { 0 } ), rem(0), pc(0), compare_flag(false),
    program_blob(), heap(initial_heap_size)
{  }

const std::array<std::int_fast32_t, vm::register_count>& vm::registers() const
{ return regs; }

std::size_t vm::program_counter() const
{ return pc; }

void vm::set_program(const std::vector<unsigned char>& program)
{ program_blob = program; }

const std::vector<unsigned char>& vm::get_program() const
{ return program_blob; }

void vm::run()
{
  // core loop of our vm
  while(pc < program_blob.size())
  {
    run_next_instr();
  }
}

op_code vm::fetch_instr()
{ return byte_to_opcode(program_blob[pc++]); }

unsigned char vm::fetch8()
{
  return program_blob[pc++];
}

std::int_fast16_t vm::fetch16()
{
  // AE F0   is stored as F0 AE
  auto old = pc++;
  return ((program_blob[old] << 8) | program_blob[pc++]) & 0b1111111111111111;
}

void vm::run_next_instr()
{
  if(pc >= program_blob.size())
    return;

  auto op = fetch_instr();
  switch(op)
  {
  default:
    {
      std::cerr << "Unknown opcode: " << std::hex << static_cast<int>(op) << "\n";
      return;
    }

  case op_code::HALT:
      return;

  case op_code::LOAD:
      {
        const std::size_t reg = fetch8();
        const std::int_fast16_t num = fetch16();

        regs[reg] = num;
      } break;

  case op_code::ALLOC:
      {
        const std::size_t reg = fetch8();
        const unsigned char bytes = regs[reg];
        const std::size_t size = heap.size() + bytes;

        heap.resize(size, 0);
      } break;

  case op_code::ADD:
      {
        const std::size_t reg = fetch8();

        const std::int_fast8_t reg0 = fetch8();
        const std::int_fast8_t reg1 = fetch8();

        regs[reg] = reg0 + reg1;
      } break;

  case op_code::INC:
      {
        const std::size_t reg = fetch8();

        ++regs[reg];
      } break;

  case op_code::DEC:
      {
        const std::size_t reg = fetch8();

        --regs[reg];
      } break;

  case op_code::SHIFT_LEFT:
      {
        const std::size_t reg = fetch8();
        const std::int_fast8_t by = fetch8();

        regs[reg] = reg << by;
      } break;

  case op_code::SHIFT_RIGHT:
      {
        const std::size_t reg = fetch8();
        const std::int_fast8_t by = fetch8();

        regs[reg] = reg >> by;
      } break;

  case op_code::SUB:
      {
        const std::size_t reg = fetch8();

        const std::int_fast8_t reg0 = fetch8();
        const std::int_fast8_t reg1 = fetch8();

        regs[reg] = reg0 - reg1;
      } break;

  case op_code::MUL:
      {
        const std::size_t reg = fetch8();

        const std::int_fast8_t reg0 = fetch8();
        const std::int_fast8_t reg1 = fetch8();

        regs[reg] = reg0 * reg1;
      } break;

  case op_code::DIV:
      {
        const std::size_t reg = fetch8();

        const std::int_fast8_t reg0 = regs[fetch8()];
        const std::int_fast8_t reg1 = regs[fetch8()];

        regs[reg] = reg0 / reg1;
        rem = reg0 % reg1;
      } break;

  case op_code::JMP:
      {
        const std::size_t reg = fetch8();

        pc = regs[reg];
      } break;

  case op_code::JMPREL:
      {
        const int rel_pos = fetch8();

        pc = pc + rel_pos;
      } break;

  case op_code::JMP_CMP:
      {
        const std::int_fast8_t reg = fetch8();

        if(compare_flag)
          pc = pc + reg;
      } break;

  case op_code::JMP_NCMP:
      {
        const std::int_fast8_t reg = fetch8();

        if(!compare_flag)
          pc = pc + reg;
      } break;

  case op_code::EQUAL:
      {
        const std::int_fast8_t reg0 = regs[fetch8()];
        const std::int_fast8_t reg1 = regs[fetch8()];

        compare_flag = (reg0 == reg1);
      } break;

  case op_code::GREATER:
      {
        const std::int_fast8_t reg0 = regs[fetch8()];
        const std::int_fast8_t reg1 = regs[fetch8()];

        compare_flag = (reg0 > reg1);
      } break;

  case op_code::LESS:
      {
        const std::int_fast8_t reg0 = regs[fetch8()];
        const std::int_fast8_t reg1 = regs[fetch8()];

        compare_flag = (reg0 < reg1);
      } break;

  case op_code::GREATER_EQUAL:
      {
        const std::int_fast8_t reg0 = regs[fetch8()];
        const std::int_fast8_t reg1 = regs[fetch8()];

        compare_flag = (reg0 >= reg1);
      } break;

  case op_code::LESS_EQUAL:
      {
        const std::int_fast8_t reg0 = regs[fetch8()];
        const std::int_fast8_t reg1 = regs[fetch8()];

        compare_flag = (reg0 <= reg1);
      } break;
  }
}

