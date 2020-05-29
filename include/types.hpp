#pragma once

#include <ir_nodes.hpp>
#include <symbol.hpp>

#include <cassert>


struct data_constructors
{
  std::vector<IRData> data;
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

  std::size_t hash(IRNodeKind kind, const IRData& dat) const
  {
    std::size_t hash = static_cast<std::size_t>(kind);

    hash ^= dat.name.get_hash() + 0x9e3779b9 + (hash<<6) + (hash>>2);

    for(auto& x : dat.args)
    {
      assert(x < hashes.size() && "Type wasn't hashed in before!");

      hash ^= hashes[x] + 0x9e3779b9 + (hash<<6) + (hash>>2);
    }
    return hash;
  }
  std::size_t add(IRNodeKind kind, IRData&& dat, IRDebugData&& debug)
  {
    auto hs = hash(kind, dat);

    if(auto it = std::find(hashes.begin(), hashes.end(), hs);
        it != hashes.end())
    {
      return it - hashes.begin();
    }

    kinds.insert(kinds.end(), kind);
    data.emplace(data.end(), std::move(dat));
    dbg_data.emplace(dbg_data.end(), std::move(debug));
    hashes.emplace(hashes.end(), hs);

    return kinds.size() - 1;
  }

  std::uint_fast32_t subst(std::uint_fast32_t in, std::uint_fast32_t what, std::uint_fast32_t with);

  std::vector<IRNodeKind> kinds;
  std::vector<IRData> data;
  std::vector<IRDebugData> dbg_data;
  tsl::robin_map<std::uint_fast32_t, data_constructors> constructors;

  std::vector<std::size_t> hashes;
};

