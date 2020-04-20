#include <diagnostic.hpp>
#include <stream_lookup.hpp>
#include <fixit_info.hpp>

#include <fmt/color.h>

#include <algorithm>
#include <cassert>

namespace mk_diag
{

json::json warn(const source_range& range,
                std::uint_fast16_t hrc, const std::string_view& message)
{
  json::json j;

  j["range"] = range;

  j["level"] = diag_level::warn;

  j["hrc"] = hrc;
  j["message"] = message;

  return j;
}

json::json error(const source_range& range,
                 std::uint_fast16_t hrc, const std::string_view& message)
{
  json::json j;

  j["range"] = range;

  j["level"] = diag_level::error;

  j["hrc"] = hrc;
  j["message"] = message;

  return j;
}

json::json fixit(const source_range& range, const fixit_info& info,
                 const std::string_view& message)
{
  json::json j;

  j["range"] = range;

  j["level"] = diag_level::fixit;

  j["message"] = message;
  j["fixit"] = info;

  return j;
}

}

diagnostics_manager::~diagnostics_manager()
{ assert(printed && "Messages have been printed."); }

diagnostics_manager& diagnostics_manager::operator<<=(const json::json& msg)
{
  std::lock_guard<std::mutex> guard(mut);

  if(msg["level"].get<diag_level>() == diag_level::error)
    err = 1;

  auto row = msg["range"]["row_beg"].get<std::size_t>();
  auto col = msg["range"]["col_beg"].get<std::size_t>();

  data[::detail::make_position(symbol(msg["range"]["module"].get<std::string_view>().data()), row, col)].push_back(msg);

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
          v["range"]["module"].get<std::string_view>(),
          v["range"]["row_beg"].get<std::uint_fast32_t>(),
          v["range"]["col_beg"].get<std::uint_fast32_t>());


      auto lv = v["level"].get<diag_level>();

      switch(lv)
      {
      default:
      case diag_level::error:
        {
          fmt::print(file, fg(fmt::color::cornsilk), "(HE-{}) ", v["hrc"].get<std::uint_fast16_t>());
          fmt::print(file, fmt::emphasis::bold | fg(fmt::color::red), "error: ");
          fmt::print(file, fmt::emphasis::bold | fg(fmt::color::white), "{}", v["message"].get<std::string_view>());
        } break;

      case diag_level::info:
        {
          fmt::print(file, fmt::emphasis::bold | fg(fmt::color::gray), "info: ");
          fmt::print(file, fmt::emphasis::bold | fg(fmt::color::white), "{}", v["message"].get<std::string_view>());
        } break;

      case diag_level::warn:
        {
          fmt::print(file, fg(fmt::color::cornsilk), "(HE-{}) ", v["hrc"].get<std::uint_fast16_t>());
          fmt::print(file, fmt::emphasis::bold | fg(fmt::color::alice_blue), "warning: ");
          fmt::print(file, fmt::emphasis::bold | fg(fmt::color::white), "{}", v["message"].get<std::string_view>());
        } break;

      case diag_level::fixit:
        {
          fmt::print(file, fmt::emphasis::bold | fg(fmt::color::sea_green), "fixit: ");
          fmt::print(file, fg(fmt::color::white), "{}", v["message"].get<std::string_view>());
          // TODO: can be made faster by doing only one pass over the code for *all* messages instead of for every
          auto info = v["fixit"].get<fixit_info>();

          auto& ifile = stream_lookup[v["range"]["module"].get<std::string_view>()];
          std::size_t row = 0;
          for(auto& correction : info.changes)
          {
            std::string buf;
            while(row < correction.first.row && ifile.good())
            { std::getline(ifile, buf); row++; }

            //ifile.seekg(std::ios::beg);
            //for(; row < correction.first.row - 1; ++row){
            //    ifile.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
            //}

            if(buf.begin() != buf.begin() + correction.first.column)
            {
              std::string first_half(buf.begin(), buf.begin() + correction.first.column);
              fmt::print(file, fg(fmt::color::white), "{}", first_half);
            }

            fmt::print(file, fg(fmt::color::light_green) | bg(fmt::color::dark_magenta), "{}", correction.second);

            if(buf.begin() + correction.first.column != buf.end())
            {
              std::string second_half(buf.begin() + correction.first.column, buf.end());
              fmt::print(file, fg(fmt::color::white), "{}", second_half);
            }

            // TODO: perhaps print some context somehow?
          }
          stream_lookup.drop(v["range"]["module"].get<std::string_view>());
        } break;
      }
      fmt::print(file, fg(fmt::color::white), "\n");
    }
  }
  printed = true;
}

int diagnostics_manager::error_code() const
{
  return err;
}

