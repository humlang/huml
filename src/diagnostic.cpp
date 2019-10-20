#include <diagnostic.hpp>

#include <fmt/format.h>
#include <fmt/color.h>

#include <algorithm>
#include <cassert>

bool diagnostic_message::print(std::FILE* file) const
{
  fmt::print(file, fmt::emphasis::bold | fg(fmt::color::white), "{}:{}:{}: ",
      location.module,
      location.row_beg, location.column_beg);

  if(diagnostic.print_codes())
  {
    fmt::print(file, fg(fmt::color::cornsilk), "(HE-{}) ", data.human_referrable_code);
  }

  switch(data.level)
  {
  default:
  case diag_level::error: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::red), "error: "); break;

  case diag_level::info: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::gray), "info: "); break;

  case diag_level::warn: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::alice_blue), "warning: "); break;

  case diag_level::fixit: fmt::print(file, fmt::emphasis::bold | fg(fmt::color::sea_green), "fixit: "); break;
  }

  fmt::print(file, fmt::emphasis::bold | fg(fmt::color::white), "{}", data.msg_template);
  // TODO: how to generically reference needed data for diagnostic?
  
  
  fmt::print(file, fg(fmt::color::white), "\n");
  return data.level == diag_level::error ? false : true;
}

diagnostic_message operator+(diagnostic_db::diag_db_entry data, source_range range)
{ return diagnostic_message(range, data, {}); }

diagnostic_message operator+(source_range range, diagnostic_db::diag_db_entry data)
{ return diagnostic_message(range, data, {}); }

diagnostic_message& operator|(diagnostic_message& msg0, diagnostic_message msg1)
{
  msg0.sub_messages.push_back(msg1);
  return msg0;
}

diagnostics_manager::~diagnostics_manager()
{
  assert(printed && "Messages have been printed.");
}

diagnostics_manager& diagnostics_manager::operator<<=(diagnostic_message msg)
{
  std::lock_guard<std::mutex> guard(mut);

  data.push_back(msg);

  std::sort(data.begin(), data.end(), [](const diagnostic_message& a, const diagnostic_message& b)
                                      { return std::string_view(a.location.module) < std::string_view(b.location.module); });

  return *this;
}

void diagnostics_manager::do_not_print_codes()
{
  std::lock_guard<std::mutex> guard(mut);
  _print_codes = false;
}

bool diagnostics_manager::print_codes() const
{
  return _print_codes;
}

void diagnostics_manager::print(std::FILE* file)
{
  if(printed)
    return;
  std::lock_guard<std::mutex> guard(mut);
  diagnostic_db::ignore ign;
  for(auto& v : data)
  {
    if(!ign.has(v.data.level))
    {
      if(!(v.print(file)))
        err = 1;
      ign = v.data.ign;
    }
  }
  printed = true;
}

int diagnostics_manager::error_code() const
{
  return err;
}

