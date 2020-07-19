#include <repl.hpp>
#include <reader.hpp>
#include <assembler.hpp>
#include <diagnostic.hpp>
#include <type_checking.hpp>

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

template struct base_repl<huml::REPL>;
template struct base_repl<virt::REPL>;


namespace huml
{
  REPL::REPL(std::string_view t) : base_repl()
  {
    if(t == "STDIN")
      return;
    auto w = huml_reader::read<huml_ast>(t);

    auto& global_ir = w.back();
    if(!diagnostic.empty())
    {
      diagnostic.print(stdout);
      diagnostic.reset();
    }
    for(auto& r : global_ir.data)
    {
      auto typ = checker.find_type(tctx, r);
      if(!diagnostic.empty())
      {
        diagnostic.print(stdout);
        diagnostic.reset();
      }
      else
      {
        global_ir.print(std::cout, typ);
        std::cout << "\n";
      }
    }
    ast.data.insert(ast.data.end(), global_ir.data.begin(), global_ir.data.end());
  }

  void REPL::run_impl()
  {
  std::cout << 
    /*
R"(
    ─┐   ╷   ┌─┐ ┌┐╷ ┌─┐
    ┌┘ ─ │   ├─┤ │││ │┌┐
    •    └── ╵ ╵ ╵└┘ └─┘
)"
*/
R"(
           \\//  //         ╷   ┌─┐ ┌┐╷ ┌─┐
  .-''-_.. (|||>(°>   [-->  │   ├─┤ │││ │┌┐
            " "             └── ╵ ╵ ╵└┘ └─┘
)";

    while(!stopped)
    {
      if(!buf_upto_semi.empty())
        std::cout << "><(....;)°> ";
      else
        std::cout << "><(typ-c)°> ";

      std::string line;
      std::getline(std::cin, line);

      process_command(line);
    }
  }

  void REPL::process_command(const std::string& line)
  {
    bool failed = false;
    struct Guard
    { Guard(bool& f, std::size_t& in) : f(f), in(in) {}
      ~Guard() { if(f) in++; else in = 0; }

      bool& f;
      std::size_t& in;
    };
    Guard guard(failed, failed_inputs);

    if(line == "'quit" || std::cin.eof())
      quit();
    else if(line == "'history")
      history();
    else if(line == "'clear-context")
    {
      tctx.data.clear();
    }
    else
    {
      // Split according to ;
      // move those with ; down and process them, otherwise store in buf_upto_semi until we get the next semicolon.
      // TODO
      std::string to_compute = buf_upto_semi + " ";
      for(std::size_t it = 0; it != std::string::npos;)
      {
        auto next = line.find(';', it + 1);

        if(next != std::string::npos)
        {
          buf_upto_semi.clear();
          to_compute += line.substr(it, next - it + 1);
          next++;
        }
        else
          buf_upto_semi += line.substr(it, line.size());

        it = next;
      }
      if(to_compute.empty())
        return;

      auto global_ir = huml_reader::read_text(to_compute);

      if(global_ir.data.empty())
      {
        if(failed_inputs > 1)
        {
          std::cout << "Type \"'quit\" or hit Ctrl-D to quit.\n";
        }
        failed = true;
        diagnostic.reset();
        return;
      }
      if(!diagnostic.empty())
      {
        diagnostic.print(stdout);
        diagnostic.reset();
      }
      ast.data.insert(ast.data.end(), global_ir.data.begin(), global_ir.data.end());
      ast.consider_scoping(); // <- rewire identifiers
      for(auto& r : global_ir.data)
      {
        if(r == nullptr)
          continue;

        auto typ = checker.find_type(tctx, r);
        if(!diagnostic.empty())
        {
          diagnostic.print(stdout);
          diagnostic.reset();
        }

        if(typ == nullptr)
          std::cout << "Could not synthesize type.\n";
        else
        {
          if(r->kind == ASTNodeKind::expr_stmt)
          {
            global_ir.print_as_type(std::cout, typ);
            std::cout << "\n";
          }
        }
      }
    }
    commands.emplace_back(line);
  }
}


namespace virt
{
  REPL::REPL() : base_repl(), virt_mach()
  {  }

  void REPL::run_impl()
  {
    std::cout << ">> Welcome to huml-vm. Happy Hacking!\n";

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

