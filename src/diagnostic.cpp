#include <diagnostic.hpp>

#include <fmt/format.h>
#include <fmt/color.h>

diagnostic_message::diagnostic_message(diagnostic_message::level lv, source_range loc, symbol msg)
  : lv(lv), location(loc), message(msg)
{  }

void diagnostic_message::print(std::FILE* file) const
{

  fmt::print(file, fmt::emphasis::bold | fg(fmt::color::white), "{}:{}:{}: ",
      location.module,
      location.row_beg, location.column_beg);

  const std::string type = (lv == level::info ? "info"
      : (lv == level::warn ? "warning"
      : (lv == level::error ? "error"
      : (lv == level::fixit ? "fixit" : "undefined"))));
  switch(lv)
  {
  default:
  case level::error: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::red), "error: ", type); break;

  case level::info: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::gray), "info: ", type); break;

  case level::warn: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::alice_blue), "warning: ", type); break;

  case level::fixit: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::sea_green), "fixit: ", type); break;
  }

  fmt::print(file, fmt::emphasis::bold | fg(fmt::color::white), "{}\n", message.get_string());

  // TODO: print context?
}

diagnostics_manager::~diagnostics_manager()
{
  for(auto& v : data)
    v.print(file);
}

diagnostics_manager& diagnostics_manager::operator<<=(diagnostic_message msg)
{
  data.push_back(msg);

  return *this;
}

