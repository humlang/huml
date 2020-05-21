#pragma once

#include <nlohmann/json.hpp>

#include <string_view>
#include <cstdio>
#include <vector>

enum class emit_classes
{
  undef,
  help,
  tokens,
  ir_print,
  ir_graph
};

NLOHMANN_JSON_SERIALIZE_ENUM( emit_classes, {
  { emit_classes::undef, "undef" },
  { emit_classes::help, "help" },
  { emit_classes::tokens, "tokens" },
  { emit_classes::ir_print, "ir-print" },
  { emit_classes::ir_graph, "ir-graph" },
})

const static auto emit_classes_list = {
  emit_classes::help,
  emit_classes::tokens,
  emit_classes::ir_print,
  emit_classes::ir_graph
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

