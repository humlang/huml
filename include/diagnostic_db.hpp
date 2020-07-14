#pragma once

#include <stream_lookup.hpp>
#include <diagnostic.hpp>
#include <symbol.hpp>

#include <fmt/color.h>

namespace diagnostic_db
{

#define db_entry(lv, name, txt) static const auto name = [](const source_range& range) \
{ return mk_diag::lv(range, __COUNTER__, txt); }

#define db_entry_arg(lv, name, txt) static const auto name = [](const source_range& range, auto t) \
{ return mk_diag::lv(range, __COUNTER__, format(FMT_STRING(txt), t)); }

#define db_entry_arg2(lv, name, txt) static const auto name = [](const source_range& range, auto t1, auto t2) \
{ return mk_diag::lv(range, __COUNTER__, format(FMT_STRING(txt), t1, t2)); }

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
db_entry_arg(error, map_impl_expected, "\"map_impl\" expected, instead got \"{}\".");
db_entry_arg(error, directive_expects_hashtag, "Directive expects \"#\", instead got \"{}\".");
db_entry_arg(error, implicit_or_explicit_expected, "\"Implicit\" or \"Explicit\" expected, instead got \"{}\".");
db_entry_arg(error, assign_expects_equal, "Assign statment expects an equal here, instead got \"{}\".");
db_entry_arg(error, unknown_token, "Can not tokenize \"{}\".");
db_entry_arg(error, not_a_prefix, "Token \"{}\" is not a prefix.");
db_entry_arg(error, not_a_sort, "Token \"{}\" is not a sort.");
db_entry_arg(error, leading_zeros, "Leading zeros for literal number \"{}\".");
db_entry_arg(error, no_number, "\"{}\" is not a number.");
db_entry_arg(error, function_expects_colon_eq, "Function expects \":=\", instead got \"{}\".");
db_entry_arg(error, expected_keyword_Prop, "Keyword \"Prop\" expected, instead got \"{}\".");
db_entry_arg(error, expected_keyword_Kind, "Keyword \"Kind\" expected, instead got \"{}\".");
db_entry_arg(error, expected_keyword_Type, "Keyword \"Type\" expected, instead got \"{}\".");
db_entry_arg(error, expected_keyword_Trait, "Keyword \"Trait\" expected, instead got \"{}\".");
db_entry_arg(error, expected_keyword_top, "Keyword \"TOP\" expected, instead got \"{}\".");
db_entry_arg(error, expected_keyword_bot, "Keyword \"BOT\" expected, instead got \"{}\".");
db_entry_arg(error, tuple_or_unit_expr_expect_lparen, "Tuple or unit expression expected a '(', instead got \"{}\".");
db_entry_arg(error, tuple_expects_comma, "Tuple expects a comma here, instead got \"{}\".");
db_entry_arg(error, tuple_expects_closing_paranthesis, "Tuple expected a closing parenthesis, instead got \"{}\".");
db_entry_arg(error, type_ctor_param_expects_closing_paranthesis, "Parameter of type constructor expects a closing parenthesis, instead got \"{}\".");
db_entry_arg(error, type_ctor_param_expects_colon, "Parameter of type constructor expects a \":\", instead got \"{}\".");
db_entry_arg(error, lambda_param_type_decl_expects_colon, "Parameter of lambda expects a \":\" here to mark its type, instead got \"{}\".");
db_entry_arg(error, lambda_expects_arrow, "Lambda expression expects an \"->\" here, instead got \"{}\".");
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
db_entry_arg(error, statement_expects_semicolon_at_end, "Statement expects semicolon at end, instead got \"{}\".");
db_entry_arg(error, closing_parenthesis_expected, "Closing paranthesis expected here, instead got \"{}\".");

db_entry_arg(error, type_keyword_expected, "\"type\" keyword expected here.");
db_entry_arg(error, type_assign_expects_colon, "Type-assign statment expects a colon here, instead got \"{}\".");
db_entry_arg(error, type_expects_pipe, "Type declaration expects pipe to define multiple constructors, instead got \"{}\".");
db_entry_arg(error, constructor_expected, "Constructor expected, instead got \"{}\".");
db_entry(error, type_check_inside_type_check, "Type checking inside a type check is forbidden.");
db_entry_arg(error, type_expects_lparen, "Type expects a `(` here, instead got \"{}\".");
db_entry_arg(error, pi_expects_lparen, "Pi type expects a `(` here, instead got \"{}\".");
db_entry_arg(error, pi_expects_rparen, "Pi type expects a `)` here, instead got \"{}\".");
db_entry_arg(error, pi_requires_explicit_domain, "Pi type requires a domain introduced with a `:` type check, instead got \"{}\".");

}

namespace sema
{
db_entry_arg(error, disallow_recursion, "\"{}\" was seen before and must not occur in this statement again.");
db_entry_arg(error, id_not_in_context, "\"{}\" was never seen before.");
db_entry_arg(error, not_wellformed, "\"{}\" is not wellformed.");
db_entry_arg2(error, not_a_subtype, "\"{}\" is not a subtype of \"{}\".");
db_entry_arg2(error, free_var_in_type, "\"{}\" must not occur in \"{}\".");
db_entry_arg2(error, cannot_unify_existentials, "\"{}\" and \"{}\" are different, cannot unify them.");
db_entry_arg(error, existential_not_in_context, "\"{}\" is not in the typing context.");
db_entry_arg(error, not_invokable, "\"{}\" is not an invokable type.");
db_entry_arg(error, unsolved_ex, "Cannot resolve dummy types for inferred type \"{}\". Add an annotation.");
db_entry_arg(error, not_a_trait, "\"{}\" is not of trait type.");
db_entry(error, trait_needs_at_least_one_param, "trait declaration needs at least one parameter.");
}


#undef db_entry
#undef db_entry_arg

}

