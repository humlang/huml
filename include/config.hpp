#pragma once

#include <nlohmann/json.hpp>

#include <string_view>
#include <cstdio>
#include <vector>

enum class emit_classes
{
  undef,
  help,
  repl,
  tokens,
  ast_print,
  ast_typecheck,
  cogen,
};

NLOHMANN_JSON_SERIALIZE_ENUM( emit_classes, {
  { emit_classes::undef, "undef" },
  { emit_classes::help, "help" },
  { emit_classes::repl, "repl" },
  { emit_classes::tokens, "tokens" },
  { emit_classes::ast_print, "ast-print" },
  { emit_classes::ast_typecheck, "ast-typecheck" },
  { emit_classes::cogen, "cogen" },
})

const static auto emit_classes_list = {
  emit_classes::help,
  emit_classes::repl,
  emit_classes::tokens,
  emit_classes::ast_print,
  emit_classes::ast_typecheck,
  emit_classes::cogen,
};

void print_emit_classes(std::FILE* f);

struct config_t
{
  bool print_help { false };

  emit_classes emit_class { emit_classes::help };
  std::size_t num_cores { 1 };

  std::vector<std::string_view> files;
  std::string output_file { "a.out" };
};

inline config_t config;

