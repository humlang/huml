#include <arguments_parser.hpp>
#include <diagnostic.hpp>
#include <config.hpp>
#include <reader.hpp>
#include <token.hpp>
#include <ast.hpp>

#include <ast_printer.hpp>

#include <iostream>
#include <sstream>
#include <cstdio>
#include <string>
#include <vector>

struct print
{
  void operator()(std::string str)
  {
    std::cout << str << "\n";
  }
};

int main(int argc, const char** argv)
{
  arguments::parse(argc, argv, stdout);

  if(config.print_help)
    return 0;

  auto w = reader::read("STDIN");
  for(auto& v : w)
  {
    std::visit(ast_printer<print>, v);
  }

  diagnostic.print(stdout);
  return diagnostic.error_code();
}

