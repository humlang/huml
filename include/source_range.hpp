#pragma once

#include <nlohmann/json.hpp>
#include <cstdint>
#include <ostream>

struct untied_source_pos
{
  std::size_t column;
  std::size_t row;
};
inline bool operator<(const untied_source_pos& lhs, const untied_source_pos& rhs)
{
  return lhs.row < rhs.row && lhs.column < rhs.column;
}
void to_json(nlohmann::json& j, const untied_source_pos& s);
void from_json(const nlohmann::json& j, untied_source_pos& s);

struct source_range
{
  std::string_view module;

  std::size_t column_beg;
  std::size_t row_beg;

  std::size_t column_end;
  std::size_t row_end;

  source_range() = default;

  source_range(std::string_view module, std::size_t column_beg, std::size_t row_beg,
                                  std::size_t column_end, std::size_t row_end);

  source_range& widen(const source_range& range);
  source_range& operator+=(const source_range& range);

  std::string to_string() const;

  untied_source_pos fst_proj() const
  { return untied_source_pos { column_beg, row_beg }; }

  untied_source_pos snd_proj() const
  { return untied_source_pos { column_end, row_end }; }

  friend std::ostream& operator<<(std::ostream& os, const source_range& src_range);
};

source_range operator+(const source_range& left, const source_range& right);

void to_json(nlohmann::json& j, const source_range& s);
void from_json(const nlohmann::json& j, source_range& s);

