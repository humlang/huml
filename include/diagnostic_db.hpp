#pragma once

#include <stream_lookup.hpp>
#include <diagnostic.hpp>
#include <symbol.hpp>

#include <fmt/color.h>

namespace diagnostic_db
{

constexpr static const auto source_information = [](auto loc) {
  if(loc.column_beg == loc.column_end)
    return make_db_entry(diag_level::info, "SC-IN-770", [](auto x) { fmt::print("end of file reached"); }, {});
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

      // print marker
      fmt::print(file, fmt::emphasis::bold | fg(fmt::color::forest_green), std::string(loc.column_end - loc.column_beg, '^'));

      // we don't need the stream anymore
      stream_lookup.drop(loc.module);
    }, {});
};

  namespace args
  {

static const auto unknown_arg = make_db_entry(diag_level::error, "AR-UN-000", "Unknown command line argument!", {});

static const auto emit_not_present = make_db_entry(diag_level::error, "AR-EM-040", "Selected emit class is unknown!", {});

static const auto num_cores_too_small = make_db_entry(diag_level::error, "AR-NC-577", "Number of cores smaller than one.", {});
static const auto num_cores_too_large = make_db_entry(diag_level::warn, "AR-NC-775", "Number of cores bigger than the number of concurrent threads supported by the implementation.", {});


  }

  namespace parser
  {

static const auto empty_module = make_db_entry(diag_level::error, "PA-EM-000", "Module must not be empty.", {});

constexpr static const auto unknown_token = [](auto t) {
  symbol err(format(FMT_STRING("Can not tokenize \"{}\"."), t));
  return make_db_entry(diag_level::error, "PA-UT-000", err.get_string(), {});
};

constexpr static const auto statements_expect_semicolon = [](auto t) {
  symbol err(format(FMT_STRING("Statement expected semicolon at its end, instead got \"{}\"."), t));
  return make_db_entry(diag_level::error, "PA-UT-000", err.get_string(), {});
};

constexpr static const auto leading_zeros = [](auto t) {
  symbol err(format(FMT_STRING("Leading zeros for literal number \"{}\"."), t));
  return make_db_entry(diag_level::error, "PA-LZ-000", err.get_string(), {});
};

constexpr static const auto no_number = [](auto t) {
  symbol err(format(FMT_STRING("\"{}\" is not a number."), t));
  return make_db_entry(diag_level::error, "PA-NN-000", err.get_string(), {});
};

constexpr static const auto unknown_keyword = [](auto t) {
  symbol err(format(FMT_STRING("Can not tokenize \"{}\"."), t));
  return make_db_entry(diag_level::error, "PA-UK-000", err.get_string(), {});
};

constexpr static const auto block_expects_lbrace = [](auto t) {
  symbol err(format(FMT_STRING("\'{{\' expected before block, instead got \"{}\"."), t));
  return make_db_entry(diag_level::error, "PA-BR-000", err.get_string(), {});
};

constexpr static const auto block_expects_rbrace = [](auto t) {
  symbol err(format(FMT_STRING("\'}}\' expected after block, instead got \"{}\"."), t));
  return make_db_entry(diag_level::error, "PA-BR-001", err.get_string(), {});
};

constexpr static const auto assign_expects_colon_equal = [](auto t) {
    symbol err(format(FMT_STRING("\":=\" expected after identifier, instead got \"{}\"."), t));
    return make_db_entry(diag_level::error, "PA-BR-002", err.get_string(), {});
};

constexpr static const auto identifier_expected = [](auto t) {
    symbol err(format(FMT_STRING("Expected identifier, instead got \"{}\"."), t));
    return make_db_entry(diag_level::error, "PA-ID-402", err.get_string(), {});
};

constexpr static const auto literal_expected = [](auto t) {
    symbol err(format(FMT_STRING("Expected literal, instead got \"{}\"."), t));
    return make_db_entry(diag_level::error, "PA-LT-402", err.get_string(), {});
};

static const auto for_expects_literal_or_id = make_db_entry(diag_level::info, "PA-FO-422",
                            "\"for\" expects literal or idenfier in loop head.", {});

  }
}

