#pragma once

#include <stream_lookup.hpp>
#include <diagnostic.hpp>
#include <symbol.hpp>

#include <fmt/color.h>

namespace diagnostic_db
{

constexpr static const auto source_information = [](auto loc) {
  return make_db_entry(diag_level::info, "SC-IN-769", 
    [loc](std::FILE* file) {
      auto& is = stream_lookup[loc.module];
      for(std::size_t i = 1; i < loc.row_beg; ++i)
      { std::string buf; std::getline(is, buf); }

      fmt::print(file, "\n");
      for(std::size_t r = loc.row_beg; r < loc.row_end; ++r)
      {
        std::string row;
        std::getline(is, row);
        fmt::print(file, fg(fmt::color::honey_dew), "> {}\n", row);
      }
      //indent
      fmt::print(file, std::string(loc.column_beg + 1, ' '));

      for(std::size_t c = loc.column_beg; c < loc.column_end; ++c)
      {
        fmt::print("^");
      }
      stream_lookup.drop(loc.module);
    }, {});
};

  namespace parser
  {

static const auto empty_module = make_db_entry(diag_level::error, "PA-EM-000", "Module must not be empty.", {});

constexpr static const auto unknown_token = [](auto t) {
  symbol err(format(FMT_STRING("Can not tokenize \"{}\"."), t));
  return make_db_entry(diag_level::error, "PA-UT-000", err.get_string(), {});
};

constexpr static const auto leading_zeros = [](auto t) {
  symbol err(format(FMT_STRING("Leading zeros for literal number \"{}\"."), t));
  return make_db_entry(diag_level::error, "PA-LZ-000", err.get_string(), {});
};

  }
}

