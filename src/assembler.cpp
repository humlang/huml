#include <assembler.hpp>
#include <reader.hpp>

ass::program ass::assembler::parse(std::string_view module)
{
  return { asm_reader::read<ass::instruction>(module) };
}

ass::program ass::assembler::parse_code(const std::string& text)
{
  return { asm_reader::read_text<ass::instruction>(text) };
}

