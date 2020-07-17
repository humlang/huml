#pragma once

#include <string>
#include <vector>

#include <type_checking.hpp>
#include <reader.hpp>
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
  void write();

  std::vector<std::string> commands;

  bool stopped { false };
};

namespace hx
{

  struct REPL : base_repl<REPL>
  {
    friend struct base_repl<REPL>;
  public:
    REPL(std::string_view t);
  private:
    void run_impl();
    void process_command(const std::string& str);
  private:
    typing_context tctx;
    hx_ast_type_checking checker;
    hx_ast ast;

    std::string buf_upto_semi;

    std::size_t failed_inputs { 0 };
  };

}


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
    void load();
  private:
    vm virt_mach;

    std::size_t failed_inputs;
  };
}

