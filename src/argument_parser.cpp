#include <arguments_parser.hpp>
#include <config.hpp>

namespace arguments
{

void parse(const std::vector<std::string>& args)
{
  // TODO: do fancy cmd args parsing
  for(auto& v : args)
  {
    if(v == "-h" || v == "-?" || v == "--help")
      config.print_help = true;
  }
}

}

