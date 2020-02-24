#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <vm.hpp>
#include <vm_opcodes.hpp>

#include <algorithm>

TEST_CASE( "vm", "" ) {
  SECTION( "base" ) {

    vm virt_mach;

    REQUIRE((std::all_of(virt_mach.registers().begin(), virt_mach.registers().end(),
                         [](std::int_fast32_t v) { return v == 0; })));
  }
  SECTION( "instr" ) {
    
    instruction instr(op_code::HALT);

    REQUIRE((instr.opcode() == op_code::HALT));
  }

  SECTION( "instructions" ) {
    const auto unkn = opcode_to_byte(op_code::UNKNOWN);
    const auto hlt = opcode_to_byte(op_code::HALT);
    const auto load = opcode_to_byte(op_code::LOAD);

    vm virt_mach;

    SECTION( "HALT" ) {
      std::vector<unsigned char> prog = { hlt, hlt, hlt, hlt };
      virt_mach.set_program(prog);

      REQUIRE((virt_mach.program_counter() == 1));
    }

    SECTION( "LOAD" ) {
      // TODO
    }

    SECTION( "JMP" ) {
      // TODO
    }

    SECTION( "JMPREL" ) {
      // TODO
    }

    SECTION( "JMP_CMP" ) {
      // TODO
    }

    SECTION( "JMP_NCMP" ) {
      // TODO
    }

    SECTION( "ADD" ) {
      // TODO
    }

    SECTION( "SUB" ) {
      // TODO
    }

    SECTION( "MUL" ) {
      // TODO
    }

    SECTION( "DIV" ) {
      // TODO
    }

    SECTION( "EQUAL" ) {
      // TODO
    }

    SECTION( "GREATER" ) {
      // TODO
    }

    SECTION( "LESS" ) {
      // TODO
    }

    SECTION( "GREATER_EQUAL" ) {
      // TODO
    }

    SECTION( "LESS_EQUAL" ) {
      // TODO
    }

    SECTION( "UNKNOWN" ) {
      std::vector<unsigned char> prog = { unkn, hlt, hlt, hlt };
      virt_mach.set_program(prog);

      REQUIRE((virt_mach.program_counter() == 0));
    }
  
  }
}
