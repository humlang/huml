#include <arguments_parser.hpp>
#include <stream_lookup.hpp>
#include <config.hpp>

#include <fmt/format.h>

#include <cx/cx_json_parser.h>

#include <cassert>

namespace arguments
{

void parse(int argc, const char** argv, std::FILE* out)
{
  detail::CmdOptions options("h-lang", "The compiler for the h-language.");
  options.add_options()
    ("h,?,help", "Prints this text.")
    ("", "Accepts arbitrary list of files.")
    ;

  auto map = options.parse(argc, argv);

  if(std::any_cast<bool>(map["h"]))
  {
    options.print_help(out);
    config.print_help = true;
  }
}

namespace detail
{

constexpr CmdOptions::CmdOptionsAdder& CmdOptions::CmdOptionsAdder::operator()(std::string_view opts, std::string_view description)
{

  return *this;
}

constexpr CmdOptions::CmdOptionsAdder CmdOptions::add_options()
{
  return {};
}

std::map<std::string, std::any> CmdOptions::parse(int argc, const char** argv)
{
  return {};
}

void CmdOptions::print_help(std::FILE* f) const
{
  fmt::print(f, "{}  -  {}\n", name, description);
}

}

}

