#include <repl.hpp>
#include <assembler.hpp>

#include <fmt/format.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>

bool prompt_yes_no()
{
  std::string answer;
  do
  {
    std::getline(std::cin, answer);
  }
  while(!(answer == "Y" || answer == "y" || answer.empty() || answer == "yes" || answer == "YES"
     || answer == "Yes" || answer == "n" || answer == "N"  || answer == "no"  || answer == "No"
     || answer == "NO"));

  return answer == "Y"   || answer == "y"   || answer.empty()
      || answer == "yes" || answer == "YES" || answer == "Yes";
}

template<typename T>
void base_repl<T>::quit()
{ stopped = true; }

template<typename T>
void base_repl<T>::history()
{
  for(const auto& cmd : commands)
    fmt::print("{}\n", cmd);
}

template<typename T>
void base_repl<T>::write()
{
  fmt::print("File: ");

  std::string filepath;
  std::getline(std::cin, filepath);

  if(filepath.empty())
    return;

  fmt::print("\nStore REPL commands? Y/n: ");
  const bool store_cmds = prompt_yes_no();

  std::fstream file(filepath, std::ios::out);
  for(const auto& str : commands)
  {
    if(str.empty() || (!store_cmds && str.front() == '\''))
      continue;

    file << str << "\n";
  }

  fmt::print("\nState written to \"{}\".\n", filepath);
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
    else if(line == "'write")
      write();
    else if(line == "'load")
      load();
    else
    {
      auto prog = ass::assembler::parse_code(line);

      const auto& v = prog.to_u8_vec();

      //auto v = parse_hex(line);

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

  void REPL::load()
  {
    fmt::print("File: ");

    std::string filepath;
    std::getline(std::cin, filepath);

    if(filepath.empty())
      return;

    std::fstream file(filepath, std::ios::in);
    for(std::string line; std::getline(file, line); process_command(line))
      ;
    fmt::print("\nState loaded from \"{}\".\n", filepath);
  }
}


