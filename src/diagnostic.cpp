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

json::json fixit(const source_range& range, const fixit_info& info)
{
  json::json j;

  j["range"] = range;

  j["level"] = diag_level::fixit;
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
          // TODO: can be made faster by doing only one pass over the code for *all* messages instead of for every
          auto info = v["fixit"].get<fixit_info>();

          auto& ifile = stream_lookup[v["range"]["module"].get<std::string_view>()];

          std::sort(info.changes.begin(), info.changes.end(), [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });
          auto correction = info.changes.begin();

          std::size_t row = 0;

          // move file pointer to line right before the first line we need to perform std::getline at
          ifile.seekg(std::ios::beg);
          if(row < correction->first.row)
          {
            for(; row + 1 < correction->first.row - 1; ++row){
              ifile.ignore(std::numeric_limits<std::streamsize>::max(),'\n');
            }
          }
          std::size_t already_printed_col = 0;
          std::string buf;
          while(correction != info.changes.end())
          {
            // fetch line. Doesn't change if two corrections are on the same row
            while(row < correction->first.row - 1 && ifile.good())
            {
              std::getline(ifile, buf);
              row++;
              already_printed_col = 0;

              // also print intermediate lines
              if(row + 1 < correction->first.row && ifile.good())
                fmt::print(file, fg(fmt::color::sandy_brown), "\n {:>7} {}", row, buf);
            }

            // print line number
            if(already_printed_col == 0)
            {
              fmt::print(file, fg(fmt::color::sandy_brown), "\n {:>7} ", row);
            }
            auto ignore_range = correction->second["what"].get<source_range>();

            //const std::size_t leftmost = std::min(correction->first.column - 1, ignore_range.column_beg);
            const std::size_t leftmost = ignore_range.column_beg;

            if(already_printed_col < leftmost - 1 && buf.size() != 0)
            {
              std::string first_half(buf.begin() + already_printed_col, buf.begin() + std::min(buf.size(), leftmost - 1));
              fmt::print(file, fg(fmt::color::white), "{}", first_half);

              already_printed_col += first_half.size();
            }

            /// Print the actual fix!!
            fmt::print(file, fg(fmt::color::light_green) | bg(fmt::color::dark_magenta),
                             "{}", correction->second["how"].get<std::string>());

//            const std::size_t rightmost = std::max(correction->first.column, ignore_range.column_end);
            const std::size_t rightmost = ignore_range.column_end;

            // only print second half if the next correction is on a different line
            if(already_printed_col < buf.size() &&
                (std::next(correction) == info.changes.end()
             || (std::next(correction) != info.changes.end() && std::next(correction)->first.row != correction->first.row)))
            {
              std::string second_half(buf.begin() + std::min(buf.size(), rightmost), buf.end());
              fmt::print(file, fg(fmt::color::white), "{}", second_half);

              already_printed_col += second_half.size();
            }
            already_printed_col += rightmost - leftmost;
            // TODO: perhaps print some context somehow? i.e. surrounding lines

            ++correction;
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

