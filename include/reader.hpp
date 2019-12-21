#pragma once

#include <source_range.hpp>
#include <symbol.hpp>
#include <ast.hpp>

#include <unordered_map>
#include <cstdio>
#include <memory>
#include <iosfwd>
#include <vector>
#include <string>

class reader
{
public:
  static std::vector<ast_type> read(std::string_view module);

  ~reader();
private:
  reader(std::string_view module);

  template<typename T>
  T get() { static_assert(sizeof(T) != 0, "unimplemented"); }
private:
  std::string_view module;
  std::istream& is;

  std::string linebuf;

  std::size_t col;
  std::size_t row;
};

