#include <arguments_parser.hpp>
#include <diagnostic_db.hpp>
#include <stream_lookup.hpp>
#include <diagnostic.hpp>
#include <config.hpp>

#include <fmt/format.h>

#include <cassert>
#include <thread>

using namespace std::string_view_literals;

void print_emit_classes(std::FILE* f)
{
  fmt::print(f, "emit classes: ");

  for(auto it = std::begin(emit_classes_list); it != std::end(emit_classes_list); ++it)
  {
    if(std::next(it) == std::end(emit_classes_list))
      fmt::print(f, "{}\n", nlohmann::json(*it).get<std::string>());
    else
      fmt::print(f, "{}, ", nlohmann::json(*it).get<std::string>());
  }
}

namespace arguments
{

void parse(int argc, const char** argv, std::FILE* out)
{
  detail::CmdOptions options("h-lang", "The compiler for the h-language.");
  options.add_options()
    ("h,?,-help", "Prints this text.", std::make_any<bool>(false), "false", [](auto x){ return std::make_any<bool>(true); })
    (",f,-files", "Accepts arbitrary list of files.", std::make_any<std::vector<std::string_view>>(), "STDIN",
      [out](auto x){ std::vector<std::string_view> w; for(auto v : x) w.push_back(v); return w; })
    ("-emit=", "Choose what to emit. Set to \"help\" to get a list.", std::make_any<emit_classes>(emit_classes::help), "help",
      [](auto x)
      {
        assert(x.size() == 1);
        auto& v = x.front();
        
        if(v.empty()) return emit_classes::help;

        nlohmann::json easy_conversion = v;
        if(easy_conversion.get<emit_classes>() != emit_classes::undef)
          return easy_conversion.get<emit_classes>();

        diagnostic <<= diagnostic_db::args::emit_not_present(source_range { "args", 0, 0, 0, 0 });
        return emit_classes::help;
      })
    ("o,-output", "Output file to write to." , std::make_any<std::string>("a.out"), "a.out",
     [](auto x)
     { return x.front(); }) // TODO: check whether we can write to that file or not?
    ("j,-num-cores", "Number of cores to use for processing modules. \"*\" to determine automatically.", std::make_any<std::size_t>(1), "1",
      [](auto x)
      {
        if(x.front() == "*")
          return static_cast<std::size_t>(std::thread::hardware_concurrency());
        auto v = static_cast<std::size_t>(std::stoull(std::string(x.front())));

        if(v == 0)
          diagnostic <<= diagnostic_db::args::num_cores_too_small(source_range { "args", 0, 0, 0, 0 });
        else if(v > std::thread::hardware_concurrency())
          diagnostic <<= diagnostic_db::args::num_cores_too_large(source_range { "args", 0, 0, 0, 0 });
        return v;
      })
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
  config.num_cores = std::any_cast<std::size_t>(map["j"]);
  config.emit_class = std::any_cast<emit_classes>(map["-emit="]);
  if(config.emit_class == emit_classes::help)
  {
    print_emit_classes(out);
    config.print_help = true;
  }
  config.output_file = std::any_cast<std::string>(map["o"]);
}

namespace detail
{

CmdOptions::CmdOptionsAdder& CmdOptions::CmdOptionsAdder::operator()(std::string_view opt_list, std::string_view description,
    std::any default_value, std::string_view default_value_str, const std::function<std::any(const std::vector<std::string_view>&)>& f)
{
  bool has_equals = false;
  std::vector<std::string_view> opts;
  auto it = opt_list.find(',');
  while(it != std::string::npos)
  {
    std::string_view opt = opt_list;
    opt.remove_suffix(opt.size() - it);
    opt_list.remove_prefix(it + 1); // + 1 to remove comma

    if(!opt.empty() && opt.back() == '=')
    {
      opt.remove_suffix(1); // <- get rid of equals
      has_equals = true;
    }
    opts.emplace_back(opt);
    it = opt_list.find(',', it);
  }
  if(!opt_list.empty())
  {
    if(!opt_list.empty() && opt_list.back() == '=')
    {
      opt_list.remove_suffix(1); // <- get rid of equals
      has_equals = true;
    }
    opts.emplace_back(opt_list);
  }

  ot->data.push_back(CmdOption { opts, description, default_value, default_value_str, f, has_equals });
  return *this;
}

CmdOptions::CmdOptionsAdder CmdOptions::add_options()
{ return { this }; }


struct CmdParse
{
  CmdParse(const std::vector<std::string_view>& args, std::map<std::string, std::any>& map, CmdOptions& cmdopts)
    : args(&args), map(&map), cmdopts(&cmdopts)
  {
    reset_cur_opt();
  }

  operator std::map<std::string, std::any>&()
  { return parse(); }
private:
  void reset_cur_opt()
  {
    cur_opt = std::nullopt;

    for(auto v : this->cmdopts->data)
    {
      for(auto f : v.opt)
      {
        if(f == "") // if we have an implicit argument, make this the initial current option
          cur_opt = v;
      }
    }
  }

  std::map<std::string, std::any>& parse()
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
    //    .... or if we have an equals option currently, since it only supports one arg
    if(((next == "" || next[0] == '-') && cur_opt.has_value()) || cur_opt->has_equals)
    {
      std::any a = (*cur_opt).parser(opt_args);
      for(auto& o : cur_opt->opt)
        (*map)[static_cast<std::string>(o) + (cur_opt->has_equals ? "=" : "")] = a;

      if(cur_opt->has_equals) // we know this has only one arg
        // TODO: generalise by just setting an argc for options
      {
        reset_cur_opt();
        opt_args.clear();
      }
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
        if(str.find(f) == 1 && str.size() - 1 == f.size()) // first char of str is `-`, after that it should match
          cur_opt = v;
      }
    }
    if(!cur_opt.has_value())
      diagnostic <<= diagnostic_db::args::unknown_arg(source_range { "args", 0, 0, 0, 0 });
    opt_args.clear();
  }

private:
  const std::vector<std::string_view>* args;
  std::map<std::string, std::any>* map; // <- not a string_view, since we need to append '=' sometimes

  CmdOptions* cmdopts;

  std::vector<std::string_view> opt_args;
  std::optional<CmdOption> cur_opt;
};

std::map<std::string, std::any> CmdOptions::parse(int argc, const char** argv)
{
  std::map<std::string, std::any> map;
  for(auto v : data)
  {
    for(auto f : v.opt)
    {
      map[static_cast<std::string>(f) + (v.has_equals ? "=" : "")] = v.default_value;
    }
  }
  if(argc - 1 == 0)
    return map;
 
  std::vector<std::string_view> args;
  args.reserve(argc - 1);

  for(int i = 1; i < argc; ++i)
  {
    // We want to split at equals
    std::string_view v = argv[i];
    if(auto it = v.rfind('='); it != std::string_view::npos && it + 1 != std::string_view::npos)
    {
      // grab the option
      auto w = v;
      w.remove_suffix(w.size() - it);
      args.push_back(w);

      // grab its argument
      v.remove_prefix(it + 1); // + 1 to remove equals
      args.push_back(v);
    }
    else
      args.push_back(argv[i]);
  }

  return CmdParse(args, map, *this);
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
    fmt::print(f, "{}    {} [default={}]\n", args, v.description, v.default_value_str);
  }
}

}

}

