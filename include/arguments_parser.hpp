#pragma once

#include <string>
#include <vector>
#include <any>
#include <map>

namespace arguments
{

void parse(int argc, const char** argv, std::FILE* out);

namespace detail
{

  struct CmdOptions
  {
  private:
    struct CmdOptionsAdder
    {
      constexpr CmdOptionsAdder& operator()(std::string_view opts, std::string_view description);
    };
  public:
    constexpr CmdOptions(std::string_view name, std::string_view description)
      : name(name), description(description)
    {  }

    constexpr CmdOptionsAdder add_options();

    std::map<std::string, std::any> parse(int argc, const char** argv);

    void print_help(std::FILE* f) const;
  private:
    std::string_view name;
    std::string_view description;
  };

}

}
