#pragma once

#include <token.hpp>
#include <cstdint>
#include <vector>

struct type_base;

enum class IRNodeKind : std::int_fast8_t
{
  undef = 0,
  literal,
  unit,
  top,
  bot,
  app,
  access,
  tuple,
  lambda,
  param,
  pattern,
  match,
  pattern_matcher,
  identifier,
  block,
  assign,
  assign_type,
  assign_data,
  expr_stmt,
  binary_exp,
  type_check,
  pi,
  Kind,
  Type,
};

struct IRData
{
  symbol symb;
  std::shared_ptr<type_base> type_annot { nullptr };
  std::uint_fast32_t other_ref { static_cast<std::uint_fast32_t>(-1) };
};

struct IRDebugData
{
  source_range loc;
};

struct hx_per_statement_ir
{
  std::vector<IRNodeKind> kinds;
  std::vector<std::uint_fast32_t> references; // stores the number of arguments per kind
  std::vector<IRData> data;
  std::vector<IRDebugData> debug_data;

  struct free_variable
  {
    std::uint_fast32_t potentially_bounded_ref { static_cast<std::uint_fast32_t>(-1) };
    std::vector<std::uint_fast32_t> refs;
  };
  symbol_map<free_variable> free_variable_roots;

  std::shared_ptr<symbol> node_name; // used for assign or type assign, otherwise it's the empty symbol


  void print(std::ostream& os) const
  {
    assert(!kinds.empty() && "IR cannot be zero, there is never an empty module!");
    print_subnode(os, 0);
  }
  std::uint_fast32_t print_subnode(std::ostream& os, std::uint_fast32_t node) const;
};

template<IRNodeKind kind>
struct IRTag
{
  std::size_t make_node(hx_per_statement_ir& cur_ir,
                        IRData&& data, IRDebugData&& debug, std::uint_fast32_t argc = 0) const
  {
    cur_ir.kinds.insert(cur_ir.kinds.end(), kind);
    cur_ir.data.emplace(cur_ir.data.end(), std::move(data));
    cur_ir.references.insert(cur_ir.references.end(), argc);

    return cur_ir.kinds.size() - 1;
  }
};

namespace IRTags
{
  static constexpr IRTag<IRNodeKind::undef> undef = {  };
  static constexpr IRTag<IRNodeKind::literal> literal = {  };
  static constexpr IRTag<IRNodeKind::unit> unit = {  };
  static constexpr IRTag<IRNodeKind::top> top = {  };
  static constexpr IRTag<IRNodeKind::bot> bot = {  };
  static constexpr IRTag<IRNodeKind::app> app = {  };
  static constexpr IRTag<IRNodeKind::access> access = {  };
  static constexpr IRTag<IRNodeKind::tuple> tuple = {  };
  static constexpr IRTag<IRNodeKind::lambda> lambda = {  };
  static constexpr IRTag<IRNodeKind::param> param = {  };
  static constexpr IRTag<IRNodeKind::pattern> pattern = {  };
  static constexpr IRTag<IRNodeKind::match> match = {  };
  static constexpr IRTag<IRNodeKind::pattern_matcher> pattern_matcher = {  };
  static constexpr IRTag<IRNodeKind::identifier> identifier = {  };
  static constexpr IRTag<IRNodeKind::block> block = {  };
  static constexpr IRTag<IRNodeKind::assign> assign = {  };
  static constexpr IRTag<IRNodeKind::assign_type> assign_type = {  };
  static constexpr IRTag<IRNodeKind::assign_data> assign_data = {  };
  static constexpr IRTag<IRNodeKind::expr_stmt> expr_stmt = {  };
  static constexpr IRTag<IRNodeKind::binary_exp> binary_exp = {  };
  static constexpr IRTag<IRNodeKind::type_check> type_check = {  };
  static constexpr IRTag<IRNodeKind::pi> pi = {  };
  static constexpr IRTag<IRNodeKind::Kind> Kind = {  };
  static constexpr IRTag<IRNodeKind::Type> Type = {  };
}

