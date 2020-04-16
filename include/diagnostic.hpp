#pragma once

#include <source_range.hpp>
#include <symbol.hpp>
#include <json.hpp>
#include <tmp.hpp>

#include <fmt/format.h>

#include <string_view>
#include <functional>
#include <vector>
#include <mutex>
#include <queue>

enum class diag_level : unsigned char
{
  error = 1,
  info  = 1 << 1,
  warn  = 1 << 2,
  fixit = 1 << 3
};

NLOHMANN_JSON_SERIALIZE_ENUM( diag_level, {
  { diag_level::error, "error" },
  { diag_level::info, "info" },
  { diag_level::warn, "warn" },
  { diag_level::fixit, "fixit" }
})

json::json mk_diag(const std::string_view& module, std::uint_fast32_t row, std::uint_fast32_t column,
                   std::uint_fast16_t hrc, diag_level lvl, const std::string_view& message);

struct diagnostics_manager
{
private:
  diagnostics_manager() {  }
public:
  ~diagnostics_manager();

  static diagnostics_manager& make()
  {
    static diagnostics_manager diag;
    return diag;
  }

  diagnostics_manager& operator<<=(const json::json& msg);

  const std::vector<json::json>& get_all() const { return data; }

  void print(std::FILE* file);
  int error_code() const;

  inline void reset() { err = 0; _print_codes = true; data.clear(); }
private:
  std::vector<json::json> data;

  int err { 0 };
  bool _print_codes { true };
  std::mutex mut;

#ifndef H_LANG_TESTING
  bool printed { false };
#else
  bool printed { true  };
#endif
};

inline diagnostics_manager& diagnostic = diagnostics_manager::make();

