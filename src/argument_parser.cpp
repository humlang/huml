#include <arguments_parser.hpp>
#include <diagnostic_db.hpp>
#include <stream_lookup.hpp>
#include <diagnostic.hpp>
#include <config.hpp>

#include <fmt/format.h>

#include <cassert>

namespace arguments
{

void parse(int argc, const char** argv, std::FILE* out)
{
  detail::CmdOptions options("h-lang", "The compiler for the h-language.");
  options.add_options()
    ("h,?,help", "Prints this text.", [](auto x){ return std::make_any<bool>(true); })
    ("", "Accepts arbitrary list of files.", [](auto){ return std::make_any<std::string>(); })
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

CmdOptions::CmdOptionsAdder& CmdOptions::CmdOptionsAdder::operator()(std::string_view opt_list, std::string_view description,
    const std::function<std::any(const std::vector<std::string>&)>& f)
{
  std::vector<std::string_view> opts;
  auto it = opt_list.find(',');
  while(it != std::string::npos)
  {
    std::string_view opt = opt_list;
    opt.remove_suffix(opt.size() - it);
    opt_list.remove_prefix(it + 1); // + 1 to remove comma

    opts.emplace_back(opt);
    it = opt_list.find(',', it);
  }
  if(!opt_list.empty())
    opts.emplace_back(opt_list);

  ot->data.push_back(CmdOption { opts, description, f });
  return *this;
}

CmdOptions::CmdOptionsAdder CmdOptions::add_options()
{ return { this }; }

std::map<std::string_view, std::any> CmdOptions::parse(int argc, const char** argv)
{
  std::map<std::string_view, std::any> map;
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
          if(str.find(f) == 1) // first char of str is `-`, after that it should match
          {
            matches = true;
            cur_opt = v;
          }
        }
      }
      if(!matches)
      {
        optargs.push_back(str);
        ++i;
      }
    }
    if(cur_opt.has_value())
    {
      std::any a = (*cur_opt).parser(optargs);
      for(auto& o : cur_opt->opt)
        map[o] = a;
    }
    else
      diagnostic <<= (-diagnostic_db::args::unknown_arg);
  }
  return map;
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

