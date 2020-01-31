#pragma once

#include <source_range.hpp>
#include <symbol.hpp>
#include <tmp.hpp>

#include <fmt/format.h>

#include <string_view>
#include <functional>
#include <vector>
#include <mutex>
#include <queue>

enum class diag_level : std::uint_fast8_t
{
  info  = 1U << 0U,
  warn  = 1U << 1U,
  error = 1U << 2U,
  fixit = 1U << 3U,
};

struct diagnostic_message;

namespace diagnostic_db
{
  struct ignore
  {
    template<typename... T>
    constexpr ignore(T... args)
      : lvs((static_cast<std::uint_fast8_t>(args) | ...))
    { static_assert((std::is_same_v<T, diag_level> && ...), "Ignoring diag_levels."); }

    constexpr ignore(diag_level lv)
      : lvs(static_cast<std::uint_fast8_t>(lv))
    {  }

    constexpr ignore()
      : lvs(0)
    {  }

    constexpr ignore& operator=(const ignore& ign)
    {
      lvs |= ign.lvs;
      return *this;
    }

    constexpr bool has(diag_level lv)
    { return (lvs & (static_cast<std::uint_fast8_t>(lv))) > 0; }

    std::uint_fast8_t lvs;
  };

  namespace detail
  {
static constexpr auto normal_print_fn_g = [](std::string_view msg)
        { return [msg](std::FILE* f) { return fmt::print(f, msg); }; };
  }

  struct diag_db_entry
  {
    std::function<void(std::FILE* file)> fn;
    std::string_view human_referrable_code;
    diag_level level;
    ignore ign;

    diag_db_entry(diag_level lv, std::string_view code, std::string_view msg)
      : fn(detail::normal_print_fn_g(msg)),
        human_referrable_code(code), level(lv), ign()
    {  }

    diag_db_entry(diag_level lv, std::string_view code, std::string_view msg, ignore ign)
      : fn(detail::normal_print_fn_g(msg)),
        human_referrable_code(code), level(lv), ign(ign)
    {  }

    diag_db_entry(diag_level lv, std::string_view code, std::function<void(std::FILE*)> f)
      : fn(f),
        human_referrable_code(code), level(lv), ign()
    {  }

    diag_db_entry(diag_level lv, std::string_view code, std::function<void(std::FILE*)> f, ignore ign)
      : fn(f),
        human_referrable_code(code), level(lv), ign(ign)
    {  }

    std::uint_fast32_t code() const
    { return hash_string(human_referrable_code); }

    diagnostic_message operator-() const;
  };
  auto make_db_entry = [](diag_level lv, std::string_view code, auto&& tmp, ignore ign)
  { return diag_db_entry(lv, code, std::forward<decltype(tmp)>(tmp), ign); };
}

struct diagnostic_source_info_t
{
  constexpr diagnostic_source_info_t(std::size_t prev_rows)
    : prev_rows(prev_rows)
  {  }

  std::size_t prev_rows;
};
constexpr auto source_context = [](std::size_t prev_rows)
  { return diagnostic_source_info_t(prev_rows); };

struct diagnostic_message
{
  void print(std::FILE* file, std::size_t indent_depth = 0) const;

  source_range location;
  diagnostic_db::diag_db_entry data;

  std::vector<diagnostic_message> sub_messages;
private:
  diagnostic_message(source_range loc, diagnostic_db::diag_db_entry entry, std::vector<diagnostic_message> v)
    : location(loc), data(entry), sub_messages(v)
  {  }

  friend diagnostic_message diagnostic_db::diag_db_entry::operator-() const;
  friend diagnostic_message operator+(diagnostic_db::diag_db_entry, source_range);
  friend diagnostic_message operator+(source_range, diagnostic_db::diag_db_entry);

  friend diagnostic_message&& operator|(diagnostic_message&& msg0, diagnostic_message&& msg1);
  friend diagnostic_message&& operator|(diagnostic_message&& msg, diagnostic_source_info_t);
};
diagnostic_message operator+(diagnostic_db::diag_db_entry data, source_range range);
diagnostic_message operator+(source_range range, diagnostic_db::diag_db_entry data);

diagnostic_message&& operator|(diagnostic_message&& msg0, diagnostic_message&& msg1);
diagnostic_message&& operator|(diagnostic_message&& msg, diagnostic_source_info_t);

inline diagnostic_message diagnostic_db::diag_db_entry::operator-() const
{
  return diagnostic_message({"unknown",0,0,0,0}, *this, {});    
}

struct diagnostics_manager
{
private:
  diagnostics_manager() {  }
public:
  ~diagnostics_manager();

  static diagnostics_manager make()
  { return diagnostics_manager(); }

  diagnostics_manager& operator<<=(diagnostic_message msg);

  void do_not_print_codes();
  bool print_codes() const;

  void print(std::FILE* file);
  int error_code() const;

  const std::vector<diagnostic_message>& get_all() const
  { /*no need to lock*/ return data; }

  inline void reset() { err = 0; _print_codes = true; data.clear(); }
private:
  std::vector<diagnostic_message> data;

  int err { 0 };
  bool _print_codes { true };
  std::mutex mut;

#ifndef H_LANG_TESTING
  bool printed { false };
#else
  bool printed { true  };
#endif
};

inline diagnostics_manager diagnostic = diagnostics_manager::make();

