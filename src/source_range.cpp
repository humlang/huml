#include "source_range.hpp"

source_range::source_range(std::string_view module, std::size_t column_beg, std::size_t row_beg,
                                                    std::size_t column_end, std::size_t row_end)
  : module(module), column_beg(column_beg), row_beg(row_beg), column_end(column_end), row_end(row_end)
{  }

void source_range::widen(const source_range& other)
{
  column_beg = std::min(column_beg, other.column_beg);
  row_beg = std::min(row_beg, other.row_beg);

  column_end = std::max(column_end, other.column_end);
  row_end = std::max(row_end, other.row_end);
}

std::string source_range::to_string() const
{
  return std::string(module) + ":"
    + std::to_string(column_beg) + ":"
    + std::to_string(row_beg);
}

std::ostream& operator<<(std::ostream& os, const source_range& src_range)
{
  return os << src_range.to_string();
}

source_range operator+(const source_range& left, const source_range& right)
{
  source_range range = left;

  range.widen(right);

  return range;
}

