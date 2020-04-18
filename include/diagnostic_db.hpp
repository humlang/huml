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
db_entry_arg(error, assign_expects_equal, "Assign statment expects an equal here, instead got \"{}\".");
db_entry_arg(error, unknown_token, "Can not tokenize \"{}\".");
db_entry_arg(error, not_a_prefix, "Token \"{}\" is not a prefix.");
db_entry_arg(error, leading_zeros, "Leading zeros for literal number \"{}\".");
db_entry_arg(error, no_number, "\"{}\" is not a number.");
db_entry_arg(error, expected_keyword_top, "Keyword \"TOP\" expected, instead got \"{}\".");
db_entry_arg(error, expected_keyword_bot, "Keyword \"BOT\" expected, instead got \"{}\".");
db_entry_arg(error, tuple_or_unit_expr_expect_lparen, "Tuple or unit expression expected a '(', instead got \"{}\".");
db_entry_arg(error, tuple_expects_comma, "Tuple expects a comma here, instead got \"{}\".");
db_entry_arg(error, tuple_expects_closing_paranthesis, "Tuple expected a closing parenthesis, instead got \"{}\".");
db_entry_arg(error, case_expects_lbracket, "Case expects an \"[\" here, instead got \"{}\".");
db_entry_arg(error, case_expects_rbracket, "Case expects an \"]\" here, instead got \"{}\".");
db_entry_arg(error, case_expects_keyword, "Keyword expected for case expression, instead got \"{}\".");
db_entry_arg(error, block_expects_semicolon, "Expected semicolon between expressions inside a block, instead got \"{}\".");
db_entry_arg(error, case_expects_pipe, "Case expects pipe to define multiple matches, instead got \"{}\".");
db_entry_arg(error, pattern_expects_double_arrow, "Pattern expects \"=>\", instead got \"{}\".");
db_entry_arg(error, block_expects_lbrace, "\'{{\' expected before block, instead got \"{}\".");
db_entry_arg(error, block_expects_rbrace, "\'}}\' expected before block, instead got \"{}\".");
db_entry_arg(error, identifier_expected, "Expected identifier, instead got \"{}\".");
db_entry_arg(error, literal_expected, "Expected literal, instead got \"{}\".");
db_entry_arg(error, invalid_identifier_for_non_pattern, "Underscore is not a valid identifier outside of a pattern.");
db_entry_arg(error, access_expects_dot, "Access expects a dot here, instead got \"{}\".");
db_entry_arg(error, lambda_expects_lambda, "Lambda expression expects `\\` as lambda symbol, instead got \"{}\".");
db_entry_arg(error, lambda_expects_dot, "Lambda expects a dot after the parameter, instead got \"{}\".");

}


#undef db_entry
#undef db_entry_arg

}

