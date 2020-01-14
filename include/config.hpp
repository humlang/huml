#pragma once

#include <string_view>
#include <vector>

struct config_t
{
  bool print_help { false };

  std::size_t num_cores { 1 };

  std::vector<std::string_view> files;
};

inline config_t config;

