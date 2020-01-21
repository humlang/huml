#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "tmp/cxmap.hpp"

TEST_CASE( "asserts for map", "[cxmap]" ) {

  constexpr map map = make_map<std::string_view, std::string_view>({{ "1", "2" }, { "2", "3" }, { "3", "4" }});
  REQUIRE(map["1"] == "2");
  REQUIRE(map.at("2") == "3");
  REQUIRE(map.empty() == false);
  
  for(auto it = map.begin(); it != map.end(); ++it)
  {
    REQUIRE((it->first == "1" || it->first == "2" || it->first == "3"));
  }

  REQUIRE(map.count("1") == 1);
  REQUIRE(map.count("x") == 0);

  REQUIRE(map.find("2") == map.begin() + 1);
  REQUIRE(map.find("22") == map.end());
}
