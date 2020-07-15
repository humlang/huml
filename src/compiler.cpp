#include <diagnostic.hpp>
#include <compiler.hpp>
#include <reader.hpp>
#include <token.hpp>
#include <repl.hpp>
#include <ast.hpp>

#include <iostream>
#include <sstream>
#include <future>
#include <thread>
#include <cstdio>
#include <string>
#include <vector>

template<bool print_newline>
void printer<print_newline>::operator()(std::string str, std::size_t indent_depth)
{
  indent(indent_depth);

  if constexpr(print_newline)
    std::cout << str << "\n";
  else
    std::cout << str;
}

template<bool print_newline>
void printer<print_newline>::indent(std::size_t depth)
{
  while(depth --> 0)
    std::cout << "  ";
}

static const std::map<emit_classes, std::function<void(std::string_view)>> emitter =
{
  { emit_classes::help, [](auto){ assert(false); } },
  { emit_classes::repl, [](std::string_view t)
    {
      hx::REPL repl(t);

      repl.run();
    } },
  { emit_classes::ast_print, [](std::string_view t)
    {
      scoping_context ctx;
      auto w = hx_reader::read<hx_ast>(t, ctx);

      if(w.empty())
        return; // <- diagnostic will contain an error
      auto& global_ir = w.back();
      
      if(diagnostic.empty() && global_ir.type_checks(ctx))
        global_ir.print(std::cout);
    } },
  { emit_classes::cogen, [](std::string_view t)
    {
      scoping_context ctx;
      auto w = hx_reader::read<hx_ast>(t, ctx);

      if(w.empty())
        return; // <- diagnostic will contain an error
      auto& global_ir = w.back();

      if(!diagnostic.empty() || !global_ir.type_checks(ctx))
        diagnostic <<= mk_diag::error(source_range{}, 1967, "Program does not typecheck.");
      else
        global_ir.cogen(config.output_file);
    } },
  { emit_classes::tokens, [](std::string_view t)
    {
      scoping_context ctx;
      auto w = hx_reader::read<token>(t, ctx);

      for(auto& tok : w)
      {
        std::cout << "Token \'" << kind_to_str(tok.kind) << "\' at " << tok.loc.to_string()
                  << " with data \"" << tok.data.get_string() << "\".\n";
      }
    } },
};

void compiler::go()
{
  std::vector<std::string_view> tasks;
  if(config.files.empty())
    tasks = { "STDIN" };
  else
    tasks = config.files;

  std::vector<std::future<void>> runners;
  for(auto tit = tasks.begin(); tit != tasks.end(); )
  {
    for(std::size_t i = runners.size(); tit != tasks.end() && i < config.num_cores; ++i)
    {
      auto& t = *tit;

      runners.emplace_back(std::async(std::launch::async, [t]() { emitter.at(config.emit_class)(t); }));

      ++tit;
    }
    for(auto rit = runners.begin(); rit != runners.end(); )
    {
      assert(rit->valid() && "Future has become invalid!");
      if(rit->wait_for(std::chrono::nanoseconds(100)) == std::future_status::ready)
        rit = runners.erase(rit);
      else
        ++rit;
    }
  }
  for(auto rit = runners.begin(); rit != runners.end(); )
  {
    assert(rit->valid() && "Future has become invalid!");
    rit->wait();
    rit = runners.erase(rit);
  }
}

