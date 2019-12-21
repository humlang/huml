#pragma once

#include <string_view>
#include <vector>

struct config_t
{
  bool print_help { false };

  std::vector<std::string_view> files;
};

inline config_t config;

