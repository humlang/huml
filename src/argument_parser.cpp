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
    ("h,?,-help", "Prints this text.", std::make_any<bool>(false), [](auto x){ return std::make_any<bool>(true); })
    (",f,-files", "Accepts arbitrary list of files.", std::make_any<std::vector<std::string_view>>(),
      [out](auto x){ std::vector<std::string_view> w; for(auto v : x) w.push_back(v); return w; })
    ;

  auto map = options.parse(argc, argv);

  if(std::any_cast<bool>(map["h"]))
  {
    options.print_help(out);
    config.print_help = true;
  }
  if(const auto& files = std::any_cast<std::vector<std::string_view>>(map["f"]); !files.empty())
  {
    config.files = files;
  }
}

namespace detail
{

CmdOptions::CmdOptionsAdder& CmdOptions::CmdOptionsAdder::operator()(std::string_view opt_list, std::string_view description,
    std::any default_value, const std::function<std::any(const std::vector<std::string_view>&)>& f)
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

  ot->data.push_back(CmdOption { opts, description, default_value, f });
  return *this;
}

CmdOptions::CmdOptionsAdder CmdOptions::add_options()
{ return { this }; }


struct CmdParse
{
  CmdParse(const std::vector<std::string_view>& args, std::map<std::string_view, std::any>& map, CmdOptions& cmdopts)
    : args(&args), map(&map), cmdopts(&cmdopts)
  {
    for(auto v : this->cmdopts->data)
    {
      for(auto f : v.opt)
      {
        if(f == "") // if we have an implicit argument, make this the initial current option
	{
          cur_opt = v;
	}
      }
    }
  }

  operator std::map<std::string_view, std::any>&()
  { return parse(); }
private:
  std::map<std::string_view, std::any>& parse()
  {
    using namespace std::literals;
    for(auto it = args->begin(); it != args->end(); ++it)
    {
      auto& str = *it;
      switch(str[0])
      {
        default:  parse_arg(str, it + 1 == args->end() ? ""sv : str); break;
        case '-': parse_option(str); break;
      }
    }

    return *map;
  }

  void parse_arg(const std::string_view& str, const std::string_view& next)
  {
    opt_args.push_back(str);

    // parse option arguments only if we hit the end or another option
    if((next == "" || next[0] == '-') && cur_opt.has_value())
    {
      std::any a = (*cur_opt).parser(opt_args);
      for(auto& o : cur_opt->opt)
        (*map)[o] = a;
    }
    // TODO: add destructor that collects all superfluous args and emits error
  }

  void parse_option(const std::string_view& str)
  {
    cur_opt = std::nullopt;
    for(auto v : cmdopts->data)
    {
      for(auto f : v.opt)
      {
        if(str.find(f) == 1) // first char of str is `-`, after that it should match
          cur_opt = v;
      }
    }
    if(!cur_opt.has_value())
      diagnostic <<= (-diagnostic_db::args::unknown_arg);
    opt_args.clear();
  }

private:
  const std::vector<std::string_view>* args;
  std::map<std::string_view, std::any>* map;

  CmdOptions* cmdopts;

  std::vector<std::string_view> opt_args;
  std::optional<CmdOption> cur_opt;
};

std::map<std::string_view, std::any> CmdOptions::parse(int argc, const char** argv)
{
  std::map<std::string_view, std::any> map;
  for(auto v : data)
  {
    for(auto f : v.opt)
    {
      map[f] = v.default_value;
    }
  }
  if(argc - 1 == 0)
    return map;
 
  std::vector<std::string_view> args(argc - 1);

  for(int i = 1; i < argc; ++i)
    args[i - 1] = argv[i];

  return CmdParse(args, map, *this);


  /*
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
  */
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

