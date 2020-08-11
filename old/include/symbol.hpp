#pragma once

#include <tsl/robin_map.h>
#include <tsl/robin_set.h>

#include <cstdint>
#include <vector>
#include <iosfwd>
#include <string>
#include <ostream>

struct symbol
{
  symbol(const std::string& str);
  symbol(const char* str);
  symbol(const symbol& s);
  symbol(symbol&& s);
  ~symbol() noexcept;

  symbol& operator=(const std::string& str);
  symbol& operator=(const char* str);
  symbol& operator=(const symbol& s);
  symbol& operator=(symbol&& s);

  friend std::ostream& operator<<(std::ostream& os, const symbol& s);

  const std::string& get_string() const;
  std::uint_fast64_t get_hash() const;
private:
  static std::string& lookup_or_emplace(std::uint_fast64_t hash, const char* str);
private:
  static tsl::robin_map<std::uint_fast64_t, std::string> symbols;

  std::uint_fast64_t hash;
};
struct symbol_hasher
{
  std::size_t operator()(symbol symb) const
  { return symb.get_hash(); }
};
struct symbol_comparer
{
  bool operator()(symbol lhs, symbol rhs) const
  { return lhs.get_hash() == rhs.get_hash(); }
};

template<class T, bool store_hash = false>
using symbol_map = tsl::robin_map<symbol, T, symbol_hasher, symbol_comparer, std::allocator<std::pair<symbol, T>>, store_hash>;

using symbol_set = tsl::robin_set<symbol, symbol_hasher, symbol_comparer, std::allocator<symbol>>;

bool operator==(const symbol& a, const symbol& b);
bool operator!=(const symbol& a, const symbol& b);
std::ostream& operator<<(std::ostream& os, const std::vector<symbol>& symbs);


