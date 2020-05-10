#pragma once

#include <config.hpp>

template<bool print_newline>
struct printer
{
  void operator()(std::string str, std::size_t indent_depth = 0);

  void indent(std::size_t depth);
};

struct compiler
{
  void go();
};

