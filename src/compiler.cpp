#include <compiler.hpp>
#include <reader.hpp>
#include <token.hpp>
#include <ast.hpp>
#include <ir.hpp>

#include <ast_printer.hpp>
#include <ast_lowering.hpp>
#include <ast_pretty_printer.hpp>

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
  { emit_classes::ast, [](std::string_view t)
    {
      auto w = hx_reader::read<scope>(t);

      for(auto& v : w[0].roots)
      {
        w[0].ast_storage.use_in(v, [&w](auto& store) {
          ast_printer<printer<true>>(w[0].ast_storage, ast_type(&store));
        });
      }
    } },
  { emit_classes::ast_pretty, [](std::string_view t)
    {
      auto w = hx_reader::read<scope>(t);

      for(auto& v : w[0].roots)
      {
        w[0].ast_storage.use_in(v, [&w](auto& store) {
          ast_pretty_printer<printer<false>>(w[0].ast_storage, ast_type(&store));
        });
      }
    } },
  { emit_classes::ir_print, [](std::string_view t)
    {
      auto w = hx_reader::read<scope>(t);

      std::vector<hx_per_statement_ir> irs;
      for(auto& v : w[0].roots)
      {
        w[0].ast_storage.use_in(v, [&irs,&w](auto& store) {
          irs.emplace_back(std::move(ast_lowering(w[0].ast_storage, ast_type(&store))));
        });
      }
      for(auto& i : irs)
      {
        i.print(std::cout);
        std::cout << "\n";
      }

      hx_ir global_ir(std::move(irs));

    } },
  { emit_classes::tokens, [](std::string_view t)
    {
      auto w = hx_reader::read<token>(t);

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
      if(rit->wait_for(std::chrono::nanoseconds(100)) == std::future_status::ready)
        rit = runners.erase(rit);
      else
        ++rit;
    }
  }
}

