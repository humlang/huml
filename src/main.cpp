#include <diagnostic.hpp>
#include <reader.hpp>
#include <token.hpp>
#include <ast.hpp>

#include <ast_printer.hpp>

#include <iostream>
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

int main(int argc, char** argv)
{
  std::vector<std::string> args;
  args.resize(argc - 1);
  for(int i = 1; i < argc; ++i)
    args.push_back(argv[i - 1]);

  auto w = reader::read("STDIN", std::cin);
  for(auto& v : w)
  {
    std::visit(ast_printer<print>, v);
  }

  diagnostic.print(stdout);
  return diagnostic.error_code();
}
