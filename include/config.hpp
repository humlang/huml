#pragma once

#include <string_view>
#include <cstdio>
#include <vector>

enum class emit_classes
{
  help,
  tokens,
  ast,
  ast_pretty,
};

void print_emit_classes(std::FILE* f);

struct config_t
{
  bool print_help { false };

  emit_classes emit_class { emit_classes::help };
  std::size_t num_cores { 1 };

  std::vector<std::string_view> files;
};

inline config_t config;

