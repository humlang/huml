#pragma once

#include <functional>
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
    std::function<std::any(const std::vector<std::string>&)> parser;
  };
  struct CmdOptions
  {
  private:
    struct CmdOptionsAdder
    {
      CmdOptionsAdder& operator()(std::string_view opts, std::string_view description, const std::function<std::any(const std::vector<std::string>&)>& f);

      CmdOptions* ot;
    };
  public:
    CmdOptions(std::string_view name, std::string_view description)
      : name(name), description(description)
    {  }

    CmdOptionsAdder add_options();

    std::map<std::string_view, std::any> parse(int argc, const char** argv);

    void print_help(std::FILE* f) const;
  private:
    std::string_view name;
    std::string_view description;

    std::vector<CmdOption> data;
  };

}

}
