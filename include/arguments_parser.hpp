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
    std::any default_value;
    std::string_view default_value_str;
    std::function<std::any(const std::vector<std::string_view>&)> parser;

    bool has_equals { false };
  };
  struct CmdOptions
  {
  private:
    friend struct CmdParse;
    struct CmdOptionsAdder
    {
      CmdOptionsAdder& operator()(std::string_view opts, std::string_view description, std::any default_value,
                                  std::string_view default_value_str,
                                  const std::function<std::any(const std::vector<std::string_view>&)>& f);

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
