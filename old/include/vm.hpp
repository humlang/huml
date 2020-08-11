#pragma once

#include <vm_opcodes.hpp>

#include <vector>
#include <array>

struct vm
{
public:
  static constexpr std::size_t register_count = 32;
public:
  vm(std::size_t initial_heap_size = 256 * 256);

  void run();

  void run_next_instr();

  std::size_t program_counter() const;
  const std::array<std::int_fast32_t, register_count>& registers() const;

  void set_program(const std::vector<unsigned char>& program);
  const std::vector<unsigned char>& get_program() const;
private:
  op_code fetch_instr();

  unsigned char fetch8();
  std::int_fast16_t fetch16();
private:
  std::array<std::int_fast32_t, register_count> regs;

  std::int_fast32_t rem;
  std::size_t pc;

  bool compare_flag;

  std::vector<unsigned char> program_blob;
  std::vector<unsigned char> heap;
};

