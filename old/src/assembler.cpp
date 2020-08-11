#include <assembler.hpp>
#include <reader.hpp>

namespace ass
{

assembler::assembler(const std::vector<ass::instruction>& v)
  : prog(v)
{  }

program assembler::parse(std::string_view module)
{
  assembler assm(asm_reader::read<ass::instruction>(module));

  assm.phase_one();
  assm.phase_two();

  return assm.prog;
}

program assembler::parse_code(const std::string& text)
{
  assembler assm(asm_reader::read_text<ass::instruction>(text));

  assm.phase_one();
  assm.phase_two();

  return assm.prog;
}


void assembler::phase_one()
{
  std::size_t pos = 0ULL;
  for(auto& i : prog.instructions)
  {
    if(i.lab)
    {
      prog.symbol_table[i.lab->data.get_hash()] = std::make_pair(symbol_type::Label, pos);
    }
    pos += 4ULL;
  }
}

void assembler::phase_two()
{
  for(auto& x : prog.instructions)
  {
    auto tmp = x.to_u8_vec(prog);

    std::move(tmp.begin(), tmp.end(), std::back_inserter(prog.bytes));
  }
}


}

