#include <arguments_parser.hpp>
#include <stream_lookup.hpp>
#include <config.hpp>

#include <fmt/format.h>

#include <cassert>

namespace arguments
{

void parse(int argc, const char** argv, std::FILE* out)
{
  detail::CmdOptions options("h-lang", "The compiler for the h-language.");
  options.add_options()
    ("h,?,help", "Prints this text.", [](auto){})
    ("", "Accepts arbitrary list of files.", [](auto){})
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

CmdOptions::CmdOptionsAdder& CmdOptions::CmdOptionsAdder::operator()(std::string_view opt_list, std::string_view description, const std::function<void(const std::vector<std::string>&)>& f)
{
  std::vector<std::string_view> opts;
  auto it = opt_list.find(',');
  while(it != std::string::npos)
  {
    std::string_view opt = opt_list;
    opt.remove_suffix(it);
    opt_list.remove_suffix(it);

    opts.emplace_back(opt);
    it = opt_list.find(',', it);
  }

  ot->data.push_back(CmdOption { opts, description, f });
  return *this;
}

CmdOptions::CmdOptionsAdder CmdOptions::add_options()
{ return { this }; }

std::map<std::string, std::any> CmdOptions::parse(int argc, const char** argv)
{
  std::map<std::string, std::any> map;
  for(int i = 1; i < argc; ++i)
  {
    std::vector<std::string> optargs;
    std::optional<CmdOption> cur_opt;
    bool matches = false;
    while(!matches && i < argc)
    {
      const std::string str = argv[i];
      for(auto v : data)
      {
        for(auto f : v.opt)
        {
          if(f == str)
          {
            matches = true;
            cur_opt = v;
          }
        }
      }
      if(!matches)
        optargs.push_back(str);
    }
    if(cur_opt.has_value())
    {
      (*cur_opt).parser(optargs);
    }
    else
    {
      // TODO: do it more smartly
      assert(false && "unrecognized command line argument");
    }
  }
  return {};
}

void CmdOptions::print_help(std::FILE* f) const
{
  fmt::print(f, "{}  -  {}\n", name, description);

  for(auto v : data)
  {
    std::string args;
    for(auto f : v.opt)
    {
      args += f;
      args += " or ";
    }
    fmt::print(f, "{}    {}\n", args, v.description);
  }
}

}

}

