#pragma once

#include <string>
#include <vector>

#include <vm.hpp>

/**
 * This REPL ("Repeat, Enter, Process"-Loop) is intended
 * to work for both hx-lang and the hx-vm.
 * It allows us to get direct feedback upon entering code.
 */

template<typename repl>
struct base_repl
{
  void run()
  { static_cast<repl&>(*this).run_impl(); }

  void quit();
  void history();

  std::vector<std::string> commands;

  bool stopped { false };
};


namespace virt
{
  struct REPL : base_repl<REPL>
  {
    friend struct base_repl<REPL>; // <- allow base_repl to call run_impl
  public:
    REPL();

  private:
    void run_impl();
    void process_command(const std::string& str);

    std::vector<unsigned char> parse_hex(const std::string& str);

    void program();
    void registers();
  private:
    vm virt_mach;

    std::size_t failed_inputs;
  };
}

