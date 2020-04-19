#include <diagnostic.hpp>

#include <fmt/color.h>

#include <algorithm>
#include <cassert>

json::json mk_diag(const std::string_view& module, std::uint_fast32_t row, std::uint_fast32_t column,
                   std::uint_fast16_t hrc, diag_level lvl, const std::string_view& message)
{
  json::json j;

  j["module"] = module;
  j["row"] = row;
  j["col"] = column;
  j["hrc"] = hrc;
  j["level"] = lvl;
  j["message"] = message;

  return j;
}

diagnostics_manager::~diagnostics_manager()
{ assert(printed && "Messages have been printed."); }

diagnostics_manager& diagnostics_manager::operator<<=(const json::json& msg)
{
  std::lock_guard<std::mutex> guard(mut);

  if(msg["level"].get<diag_level>() == diag_level::error)
    err = 1;

  auto row = msg["row"].get<std::uint_fast32_t>();
  auto col = msg["col"].get<std::uint_fast32_t>();

  data[::detail::make_position(row, col)].push_back(msg);

  return *this;
}

void diagnostics_manager::print(std::FILE* file)
{
  if(printed)
    return;
  std::lock_guard<std::mutex> guard(mut);
  for(auto& w : data)
  {
    for(auto& v : w.second)
    {
      fmt::print(file, fmt::emphasis::bold | fg(fmt::color::white), "{}:{}:{}: ",
          v["module"].get<std::string_view>(),
          v["row"].get<std::uint_fast32_t>(),
          v["col"].get<std::uint_fast32_t>());

      fmt::print(file, fg(fmt::color::cornsilk), "(HE-{}) ", v["hrc"].get<std::uint_fast16_t>());

      switch(v["level"].get<diag_level>())
      {
      default:
      case diag_level::error: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::red), "error: "); break;

      case diag_level::info: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::gray), "info: "); break;

      case diag_level::warn: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::alice_blue), "warning: "); break;

      case diag_level::fixit: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::sea_green), "fixit: "); break;
      }
      fmt::print(file, fmt::emphasis::bold | fg(fmt::color::white), "{}", v["message"].get<std::string_view>());
      fmt::print(file, fg(fmt::color::white), "\n");
    }
  }
  printed = true;
}

int diagnostics_manager::error_code() const
{
  return err;
}

