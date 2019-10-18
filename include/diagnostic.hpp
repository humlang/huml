#pragma once

#include <source_range.hpp>
#include <symbol.hpp>
#include <tmp.hpp>

#include <string_view>
#include <vector>
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

  struct diag_db_entry
  {
    std::string_view human_referrable_code;
    std::string_view msg_template;
    diag_level level;
    ignore ign;

    constexpr diag_db_entry(diag_level lv, std::string_view code, std::string_view msg)
      : human_referrable_code(code), msg_template(msg), level(lv), ign()
    {  }

    constexpr diag_db_entry(diag_level lv, std::string_view code, std::string_view msg, ignore ign)
      : human_referrable_code(code), msg_template(msg), level(lv), ign(ign)
    {  }

    constexpr std::uint_fast32_t code() const
    { return hash_string(human_referrable_code); }

    diagnostic_message operator-() const;
  };
}

struct diagnostic_message
{
  bool print(std::FILE* file) const;

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
  friend diagnostic_message& operator|(diagnostic_message& msg0, diagnostic_message msg1);
};
diagnostic_message operator+(diagnostic_db::diag_db_entry data, source_range range);
diagnostic_message operator+(source_range range, diagnostic_db::diag_db_entry data);

diagnostic_message operator|(diagnostic_message msg0, diagnostic_message msg1);

inline diagnostic_message diagnostic_db::diag_db_entry::operator-() const
{
  return diagnostic_message({"unknown",0,0,0,0}, *this, {});    
}

struct diagnostics_manager
{
public:
  ~diagnostics_manager();

  diagnostics_manager& operator<<=(diagnostic_message msg);

  void do_not_print_codes();
  bool print_codes() const;

  void print(std::FILE* file);
  int error_code() const;

  const std::vector<diagnostic_message>& get_all() const
  { return data; }
private:
  std::vector<diagnostic_message> data;

  int err { 0 };
  bool _print_codes { true };

#ifndef H_LANG_TESTING
  bool printed { false };
#else
  bool printed { true  };
#endif
};

inline thread_local diagnostics_manager diagnostic;

