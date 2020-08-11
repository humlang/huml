#include "source_range.hpp"

source_range::source_range(std::string_view module, std::size_t column_beg, std::size_t row_beg,
                                                    std::size_t column_end, std::size_t row_end)
  : module(module), column_beg(column_beg), row_beg(row_beg), column_end(column_end), row_end(row_end)
{  }

source_range& source_range::widen(const source_range& other)
{
  column_beg = std::min(column_beg, other.column_beg);
  row_beg = std::min(row_beg, other.row_beg);

  column_end = std::max(column_end, other.column_end);
  row_end = std::max(row_end, other.row_end);

  return *this;
}

source_range& source_range::operator+=(const source_range& other)
{ return this->widen(other); }

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


void to_json(nlohmann::json& j, const untied_source_pos& s)
{
  j = nlohmann::json{
    { "col", s.column },
    { "row", s.row },
  };
}

void from_json(const nlohmann::json& j, untied_source_pos& s)
{
  s = untied_source_pos {
    j["col"].get<std::size_t>(),
    j["row"].get<std::size_t>(),
  };
}


void to_json(nlohmann::json& j, const source_range& s)
{
  j = nlohmann::json{
    { "module", s.module },
    { "col_beg", s.column_beg },
    { "row_beg", s.row_beg },
    { "col_end", s.column_end },
    { "row_end", s.row_end },
  };
}

void from_json(const nlohmann::json& j, source_range& s)
{
  s = source_range { j["module"].get<std::string_view>(),
                     j["col_beg"].get<std::size_t>(),
                     j["row_beg"].get<std::size_t>(),
                     j["col_end"].get<std::size_t>(),
                     j["row_end"].get<std::size_t>()
  };
}

