#pragma once

#include <source_range.hpp>
#include <symbol.hpp>

enum class ASTNodeKind : std::int_fast8_t
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

struct ASTData
{
  constexpr static std::uint_fast32_t no_type = static_cast<std::uint_fast32_t>(-1);
  constexpr static std::uint_fast32_t no_ref  = static_cast<std::uint_fast32_t>(-1);

  symbol name;

  std::size_t argc { 0 };

  std::uint_fast32_t type_annot { no_type };

  std::uint_fast32_t back_ref { no_ref };
};

struct ASTDebugData
{
  source_range loc;
};

template<ASTNodeKind kind>
struct ASTTag
{
  template<typename Container>
  std::size_t make_node(Container& cur_ir, ASTData&& data, ASTDebugData&& debug) const
  {
    return cur_ir.add(kind, std::move(data), std::move(debug));

    return cur_ir.kinds.size() - 1;
  }

  template<typename Iterator, typename Container>
  std::size_t make_node(Iterator insert_at, Container& cur_ir, ASTData&& data, ASTDebugData&& debug) const
  {
    return cur_ir.add(std::forward<Iterator>(insert_at), kind, std::move(data), std::move(debug));

    return cur_ir.kinds.size() - 1;
  }
};

namespace ASTTags
{
  static constexpr ASTTag<ASTNodeKind::undef> undef = {  };
  static constexpr ASTTag<ASTNodeKind::unit> unit = {  };
  static constexpr ASTTag<ASTNodeKind::app> app = {  };
  static constexpr ASTTag<ASTNodeKind::lambda> lambda = {  };
  static constexpr ASTTag<ASTNodeKind::param> param = {  };
  static constexpr ASTTag<ASTNodeKind::pattern> pattern = {  };
  static constexpr ASTTag<ASTNodeKind::match> match = {  };
  static constexpr ASTTag<ASTNodeKind::pattern_matcher> pattern_matcher = {  };
  static constexpr ASTTag<ASTNodeKind::identifier> identifier = {  };
  static constexpr ASTTag<ASTNodeKind::assign> assign = {  };
  static constexpr ASTTag<ASTNodeKind::assign_type> assign_type = {  };
  static constexpr ASTTag<ASTNodeKind::assign_data> assign_data = {  };
  static constexpr ASTTag<ASTNodeKind::expr_stmt> expr_stmt = {  };
  static constexpr ASTTag<ASTNodeKind::Kind> Kind = {  };
  static constexpr ASTTag<ASTNodeKind::Type> Type = {  };
  static constexpr ASTTag<ASTNodeKind::Prop> Prop = {  };
}

