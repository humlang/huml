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
  ast,
  ast_pretty,
  ir_print
};

NLOHMANN_JSON_SERIALIZE_ENUM( emit_classes, {
  { emit_classes::undef, "undef" },
  { emit_classes::help, "help" },
  { emit_classes::tokens, "tokens" },
  { emit_classes::ast, "ast" },
  { emit_classes::ast_pretty, "ast-pretty" },
  { emit_classes::ir_print, "ir-print" },
})

const static auto emit_classes_list = {
  emit_classes::help, emit_classes::tokens,
  emit_classes::tokens, emit_classes::ast,
  emit_classes::ast_pretty, emit_classes::ir_print
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

