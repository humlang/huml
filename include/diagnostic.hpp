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

struct fixit_info;

namespace mk_diag
{
json::json error(const source_range& range,
                 std::uint_fast16_t hrc, const std::string_view& message);

json::json warn(const source_range& range,
                 std::uint_fast16_t hrc, const std::string_view& message);

json::json fixit(const source_range& range, const fixit_info& info);
}

namespace detail
{
  struct position
  {
    symbol module;
    std::size_t row;
    std::size_t col;

    bool operator==(const position& other) const
    { return module.get_hash() == other.module.get_hash() && row == other.row && col == other.col; }
  };
  inline position make_position(symbol module, std::size_t row, std::size_t col)
  {
    return position { module, row, col };
  }
}
namespace std
{
  template<>
  struct hash<::detail::position>
  {
    std::size_t operator()(const ::detail::position& p) const
    {
      return ((std::hash<std::uint_fast32_t>()(p.row)
               ^ (std::hash<std::uint_fast32_t>()(p.col) << 1)) >> 1);
    }
  };
}

struct diagnostics_manager
{
private:
  diagnostics_manager()  {  }
public:
  ~diagnostics_manager();

  static diagnostics_manager& make()
  {
    static diagnostics_manager diag;
    return diag;
  }

  diagnostics_manager& operator<<=(const json::json& msg);

  bool empty() const { return data.empty(); }

  void print(std::FILE* file);
  int error_code() const;

  inline void reset() { err = 0; _print_codes = true; data.clear(); }
private:
  tsl::robin_map<::detail::position, std::vector<json::json>> data;

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

