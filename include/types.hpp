#pragma once

#include <per_statement_ir.hpp>
#include <symbol.hpp>

#include <cassert>

enum class type_kind
{
  Kind,
  Type,
  Prop,
  TypeConstructor,
  Application,
  Id,
  Pi
};

struct TypeData
{
  symbol name;
  std::vector<std::uint_fast32_t> args {};
  std::uint_fast32_t has_type { static_cast<std::uint_fast32_t>(-1) };
};

struct data_constructors
{
  std::vector<TypeData> data;
};

// CANNOT ASSUME THAT TYPES ARE LINEAR IN MEMORY!!
//  i.e. can't do `node = print(os, node + 1);`
struct type_table
{
  type_table();

  static constexpr std::size_t Kind_sort_idx = 0;
  static constexpr std::size_t Type_sort_idx = 1;
  static constexpr std::size_t Prop_sort_idx = 2;
  static constexpr std::size_t Unit_idx = 3;

  std::vector<type_kind> kinds;
  std::vector<TypeData> data;
  tsl::robin_map<std::uint_fast32_t, data_constructors> constructors;

  void print_type(std::ostream& os, std::uint_fast32_t type_ref)
  {
    switch(type_ref)
    {
    case Kind_sort_idx: os << "Kind"; break;
    case Type_sort_idx: os << "Type"; break;
    case Prop_sort_idx: os << "Prop"; break;
    case Unit_idx:      os << "1"; break; 
    }
    assert(type_ref > Unit_idx);
    switch(kinds[type_ref])
    {
      case type_kind::TypeConstructor:
      case type_kind::Id: {
        os << data[type_ref].name.get_string();

        if(data[type_ref].has_type != static_cast<std::uint_fast32_t>(-1))
        {
          os << " : " << data[data[type_ref].has_type].name.get_string();
        }
      } break;
      case type_kind::Pi: { // TODO: Check if body is independent of arg, print `->` instead
        os << "\\ (";
        print_type(os, data[type_ref].args.front());
        os << "). ";
        print_type(os, data[type_ref].args.back());
      } break;
      case type_kind::Application: {
        os << "((";
        print_type(os, data[type_ref].args.front());
        os << ") (";
        print_type(os, data[type_ref].args.back());
        os << "))";
      }
    }
  }
};

template<type_kind kind>
struct type_tag
{
  std::uint_fast32_t make_node(type_table& typtab, TypeData&& dat) const
  {
    typtab.kinds.emplace_back(kind);
    typtab.data.emplace_back(dat);

    return typtab.kinds.size() - 1;
  }
};

namespace type_tags
{
  constexpr static type_tag<type_kind::Kind> kind = {};
  constexpr static type_tag<type_kind::Type> type = {};
  constexpr static type_tag<type_kind::Prop> prop = {};
  constexpr static type_tag<type_kind::TypeConstructor> type_constructor = {};
  constexpr static type_tag<type_kind::Application> application = {};
  constexpr static type_tag<type_kind::Id> id = {};
  constexpr static type_tag<type_kind::Pi> pi = {};
}

