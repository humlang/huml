#include <repl.hpp>

#include <fmt/format.h>

#include <iostream>
#include <iomanip>
#include <sstream>

template<typename T>
void base_repl<T>::quit()
{ stopped = true; }

template<typename T>
void base_repl<T>::history()
{
  for(const auto& cmd : commands)
    fmt::print("{}\n", cmd);
}

namespace virt
{
  REPL::REPL() : base_repl(), virt_mach()
  {  }

  void REPL::run_impl()
  {
    std::cout << ">> Welcome to hx-vm. Happy Hacking!\n";

    while(!stopped)
    {
      std::cout << "(λ) > ";

      std::string line;
      std::getline(std::cin, line);

      process_command(line);
    }
  }

  std::vector<unsigned char> REPL::parse_hex(const std::string& line)
  {
    std::vector<unsigned char> data;

    std::stringstream ss(line);
    do
    {
      std::string buf;
      std::getline(ss, buf, ' ');

      if(buf.empty())
        continue;
      try
      { data.emplace_back(std::stoi(buf, 0, 16)); }
      catch(...)
      { return {}; }
    }
    while(!(ss.eof()));

    return data;
  }

  void REPL::process_command(const std::string& line)
  {
    if(line == "'quit" || std::cin.eof())
      quit();
    else if(line == "'history")
      history();
    else if(line == "'program")
      program();
    else if(line == "'registers")
      registers();
    else if(line == "'run")
      virt_mach.run();
    else if(line == "'next")
      virt_mach.run_next_instr();
    else
    {
      auto v = parse_hex(line);

      if(v.empty())
      {
        std::cout << "Invalid command.\n";

        if(failed_inputs++ > 1)
          std::cout << "Type \"'quit\" or hit Ctrl-D to quit.\n";
      }
      else
      { virt_mach.set_program(v); }
    }
    commands.emplace_back(line);
  }

  void REPL::program()
  {
    std::cout << "Bytes of the currently loaded program:\n";

    std::size_t char_count = 0;
    for(auto& v : virt_mach.get_program())
    {
      fmt::print("{0:X}{1:X}", (v & 0b11110000) >> 4, v & 0b1111);

      if(char_count % 2 == 1)
        fmt::print(" ");

      char_count += 1;

      if(char_count + 1 >= 80)
      {
        fmt::print("\n");

        char_count = 0;
      }
    }
    if(char_count != 0)
      fmt::print("\n");
  }

  void REPL::registers()
  {
    fmt::print("Registers: [{}", virt_mach.registers().front());

    for(auto it = std::next(virt_mach.registers().begin()); it != virt_mach.registers().end(); ++it)
      fmt::print(", {}", *it);

    fmt::print("]\n");
  }
}


