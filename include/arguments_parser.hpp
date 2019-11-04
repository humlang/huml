#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <any>
#include <map>

namespace arguments
{

void parse(int argc, const char** argv, std::FILE* out);

namespace detail
{

  struct CmdOption
  {
    std::vector<std::string_view> opt;
    std::string_view description;
  };
  struct CmdOptions
  {
  private:
    struct CmdOptionsAdder
    {
      CmdOptionsAdder& operator()(std::string_view opts, std::string_view description);

      CmdOptions* ot;
    };
  public:
    CmdOptions(std::string_view name, std::string_view description)
      : name(name), description(description)
    {  }

    CmdOptionsAdder add_options();

    std::map<std::string, std::any> parse(int argc, const char** argv);

    void print_help(std::FILE* f) const;
  private:
    std::string_view name;
    std::string_view description;

    std::vector<CmdOption> data;
  };

}

}
