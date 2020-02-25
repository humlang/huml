#include <repl.hpp>

#include <iostream>

namespace virt
{
  REPL::REPL() : base_repl(), virt_mach(), stopped(false)
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

  void REPL::process_command(const std::string& line)
  {
    if(line == "quit")
      quit();
    else
    {
      std::cout << "Invalid command.\n";
    }
  }

  void REPL::quit()
  { stopped = true; }
}


