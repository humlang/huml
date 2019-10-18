#pragma once

#include <diagnostic.hpp>
#include <symbol.hpp>

#include <fmt/format.h>

namespace diagnostic_db
{

  namespace parser
  {

constexpr static const auto empty_module = diag_db_entry(diag_level::error, "PA-EM-000", "Module must not be empty.");

constexpr static const auto unknown_token = [](auto t) {
  symbol err(format(FMT_STRING("Can not tokenize \"{}\"."), t));
  return diag_db_entry(diag_level::error, "PA-UT-000", err.get_string(),
                       diagnostic_db::ignore(diag_level::error, diag_level::info));
};

constexpr static const auto leading_zeros = [](auto t) {
  symbol err(format(FMT_STRING("Leading zeros for literal number \"{}\"."), t));
  return diag_db_entry(diag_level::error, "PA-LZ-000", err.get_string(),
                       diagnostic_db::ignore(diag_level::error, diag_level::info));
};

  }
}

