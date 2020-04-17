#pragma once

#include <stream_lookup.hpp>
#include <diagnostic.hpp>
#include <symbol.hpp>

#include <fmt/color.h>

namespace diagnostic_db
{

#define db_entry(lv, name, txt) static const auto name = [](const std::string_view& module, std::uint_fast32_t row, std::uint_fast32_t column) \
{ return mk_diag(module, row, column, __COUNTER__, diag_level::lv, txt); }

#define db_entry_arg(lv, name, txt) static const auto name = [](const std::string_view& module, std::uint_fast32_t row, std::uint_fast32_t column, auto t) \
{ return mk_diag(module, row, column, __COUNTER__, diag_level::lv, format(FMT_STRING(txt), t)); }

namespace args
{

db_entry(error, unknown_arg, "Unknown command line argument!");
db_entry(error, emit_not_present, "Selected emit class is unknown!");
db_entry(error, num_cores_too_small, "Number of cores smaller than one.");
db_entry(warn, num_cores_too_large, "Number of cores bigger than the number of concurrent threads supported by the implementation.");

}

namespace parser
{

db_entry(error, empty_module, "Module must not be empty.");
db_entry_arg(error, unknown_token, "Can not tokenize \"{}\".");
db_entry_arg(error, not_a_prefix, "Token \"{}\" is not a prefix.");
db_entry_arg(error, leading_zeros, "Leading zeros for literal number \"{}\".");
db_entry_arg(error, no_number, "\"{}\" is not a number.");
db_entry_arg(error, tuple_or_unit_expr_expect_lparen, "Tuple or unit expression expected a '(', instead got \"{}\".");
db_entry_arg(error, tuple_expects_comma, "Tuple expects a comma here, instead got \"{}\".");
db_entry_arg(error, tuple_expects_closing_paranthesis, "Tuple expected a closing parenthesis, instead got \"{}\".");
db_entry_arg(error, block_expects_lbrace, "\'{{\' expected before block, instead got \"{}\".");
db_entry_arg(error, block_expects_rbrace, "\'}}\' expected before block, instead got \"{}\".");
db_entry_arg(error, identifier_expected, "Expected identifier, instead got \"{}\".");
db_entry_arg(error, literal_expected, "Expected literal, instead got \"{}\".");

}


#undef db_entry
#undef db_entry_arg

}

