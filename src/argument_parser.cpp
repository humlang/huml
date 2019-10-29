#include <arguments_parser.hpp>
#include <stream_lookup.hpp>
#include <config.hpp>

#include <cassert>

namespace arguments
{

void parse(const std::vector<std::string>& args)
{
  // TODO: do fancy cmd args parsing
  for(auto& v : args)
  {
    if(v == "-h" || v == "-?" || v == "--help")
      config.print_help = true;
    else
    {
      // must be a file or error
      std::fstream file(v, std::ios::in);
      if(!(file.good()))
      {
        // could not open file TODO
        assert(false);
      }
      else
      {
        stream_lookup[v];
        stream_lookup.drop(v);
      }
    }
  }
}

}

