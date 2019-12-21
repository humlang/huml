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

  std::vector<std::string_view> tasks;
  if(config.print_help)
    goto end;

  if(config.files.empty())
    tasks = { "STDIN" };
  else
    tasks = config.files;


  // just do it sequentially now
  for(auto& t : tasks)
  {
    auto w = reader::read(t);
    for(auto& v : w)
    {
      std::visit(ast_printer<print>, v);
    }
  }

end:
  diagnostic.print(stdout);
  return diagnostic.error_code();
}

