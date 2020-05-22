#include <compiler.hpp>
#include <reader.hpp>
#include <token.hpp>
#include <ir.hpp>

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
  { emit_classes::ir_print, [](std::string_view t)
    {
      auto w = hx_reader::read<hx_ir>(t);

      if(w.empty())
        return; // <- diagnostic will contain an error
      auto& global_ir = w.back();

      for(auto& i : w[0].nodes)
      {
        i.print(std::cout, w[0].types);
        std::cout << "\n";
      }

      if(global_ir.type_checks())
        std::cout << "### Does type check." << std::endl;
      else
        std::cout << "!!! Does not type check." << std::endl;
    } },
  { emit_classes::ir_graph, [](std::string_view t)
    {
      auto w = hx_reader::read<hx_ir>(t);

      if(w.empty())
        return; // <- diagnostic will contain an error
      auto& global_ir = w.back();

      std::cout << "digraph {\n";
      for(auto& n : global_ir.nodes)
      {
        std::cout << "  ";
        if(n.node_name)
          std::cout << n.node_name->get_string() << "\n";
        else
        {
          std::cout << "\""; n.print(std::cout, global_ir.types); std::cout << "\"\n";
        }
      }

      for(auto& e : global_ir.edges)
      {
        std::cout << "  ";
        if(global_ir.nodes[e.from].node_name)
          std::cout << global_ir.nodes[e.from].node_name->get_string();
        else
        {
          std::cout << "\""; global_ir.nodes[e.from].print(std::cout, global_ir.types); std::cout << "\"";
        }
        std::cout << " -> ";
        if(global_ir.nodes[e.to].node_name)
          std::cout << global_ir.nodes[e.to].node_name->get_string();
        else
        {
          std::cout << "\""; global_ir.nodes[e.to].print(std::cout, global_ir.types); std::cout << "\"";
        }
        std::cout << "\n";
      }
      std::cout << "}\n";
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

