#pragma once

#include <cstdint>
#include <ostream>

struct source_range
{
  const char* module;

  std::size_t column_beg;
  std::size_t row_beg;

  std::size_t column_end;
  std::size_t row_end;

  source_range() = default;

  source_range(const char* module, std::size_t column_beg, std::size_t row_beg,
                                  std::size_t column_end, std::size_t row_end);

  void widen(const source_range& range);

  std::string to_string() const;

  friend std::ostream& operator<<(std::ostream& os, const source_range& src_range);
};

source_range operator+(const source_range& left, const source_range& right);

