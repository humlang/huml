#pragma once

#include <source_range.hpp>
#include <symbol.hpp>

#include <vector>
#include <queue>

struct diagnostic_message
{
  enum class level
  {
    info,
    warn,
    error,
    fixit,
  };

  diagnostic_message(level lv, source_range loc, symbol msg);

  void print(std::FILE* file) const;

  source_range location;
  symbol message;
  level lv;

  std::vector<diagnostic_message> sub_messages;
};

struct diagnostics_manager
{
public:
  ~diagnostics_manager();

  diagnostics_manager& operator<<=(diagnostic_message msg);

  std::FILE* file;
private:
  std::vector<diagnostic_message> data;
};

static diagnostics_manager diagnostic;

