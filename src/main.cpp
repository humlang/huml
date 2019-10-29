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

void print_help();

int main(int argc, char** argv)
{
  std::vector<std::string> args;
  args.reserve(argc - 1);
  for(int i = 1; i < argc; ++i)
    args.push_back(argv[i]);

  arguments::parse(args);

  if(config.print_help)
  {
    print_help();
    return 0;
  }

  auto w = reader::read("STDIN");
  for(auto& v : w)
  {
    std::visit(ast_printer<print>, v);
  }

  diagnostic.print(stdout);
  return diagnostic.error_code();
}

void print_help()
{
  std::cout << "h-lang -[h|?|-help]\n";
  std::cout << "  Arguments:\n";
  std::cout << "    -h -? --help      Prints this text.\n";
}

