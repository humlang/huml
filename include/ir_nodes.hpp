#pragma once

#include <source_range.hpp>
#include <symbol.hpp>

enum class IRNodeKind : std::int_fast8_t
{
  undef = 0,
  Kind,
  Type,
  Prop,
  unit,
  app,
  lambda,
  param,
  pattern,
  match,
  pattern_matcher,
  identifier,
  assign,
  assign_type,
  assign_data,
  expr_stmt,
};

struct IRData
{
  constexpr static std::uint_fast32_t no_type = static_cast<std::uint_fast32_t>(-1);
  constexpr static std::uint_fast32_t no_ref  = static_cast<std::uint_fast32_t>(-1);

  symbol name;

  std::size_t argc { 0 };

  std::uint_fast32_t type_annot { no_type };

  std::uint_fast32_t back_ref { no_ref };
};

struct IRDebugData
{
  source_range loc;
};

template<IRNodeKind kind>
struct IRTag
{
  template<typename Container>
  std::size_t make_node(Container& cur_ir, IRData&& data, IRDebugData&& debug) const
  {
    return cur_ir.add(kind, std::move(data), std::move(debug));

    return cur_ir.kinds.size() - 1;
  }

  template<typename Iterator, typename Container>
  std::size_t make_node(Iterator insert_at, Container& cur_ir, IRData&& data, IRDebugData&& debug) const
  {
    return cur_ir.add(std::forward<Iterator>(insert_at), kind, std::move(data), std::move(debug));

    return cur_ir.kinds.size() - 1;
  }
};

namespace IRTags
{
  static constexpr IRTag<IRNodeKind::undef> undef = {  };
  static constexpr IRTag<IRNodeKind::unit> unit = {  };
  static constexpr IRTag<IRNodeKind::app> app = {  };
  static constexpr IRTag<IRNodeKind::lambda> lambda = {  };
  static constexpr IRTag<IRNodeKind::param> param = {  };
  static constexpr IRTag<IRNodeKind::pattern> pattern = {  };
  static constexpr IRTag<IRNodeKind::match> match = {  };
  static constexpr IRTag<IRNodeKind::pattern_matcher> pattern_matcher = {  };
  static constexpr IRTag<IRNodeKind::identifier> identifier = {  };
  static constexpr IRTag<IRNodeKind::assign> assign = {  };
  static constexpr IRTag<IRNodeKind::assign_type> assign_type = {  };
  static constexpr IRTag<IRNodeKind::assign_data> assign_data = {  };
  static constexpr IRTag<IRNodeKind::expr_stmt> expr_stmt = {  };
  static constexpr IRTag<IRNodeKind::Kind> Kind = {  };
  static constexpr IRTag<IRNodeKind::Type> Type = {  };
  static constexpr IRTag<IRNodeKind::Prop> Prop = {  };
}

