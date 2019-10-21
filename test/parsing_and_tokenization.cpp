#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <stream_lookup.hpp>
#include <diagnostic.hpp>
#include <reader.hpp>

#include <sstream>

TEST_CASE( "Numbers are parsed correctly", "[Numbers]" ) {
  
  SECTION( "single-digit" ) {

    stream_lookup.write_test("0 1 2 3 4 5 6 7 8 9");
    auto w = reader::read("TESTSTREAM");

    std::size_t i = 0;
    std::size_t col = 1;
    for(auto& v : w)
    {
      REQUIRE( std::holds_alternative<literal>(v) );

      if(std::holds_alternative<literal>(v))
      {
        auto& lit = std::get<literal>(v);

        REQUIRE( lit.tok.kind == token_kind::LiteralNumber );
        REQUIRE( lit.tok.data.get_string() == std::to_string(i) );

        REQUIRE( lit.tok.loc.module == "TESTSTREAM" );
        REQUIRE( lit.tok.loc.row_beg == 1 );
        REQUIRE( lit.tok.loc.row_end == 2 );
        REQUIRE( lit.tok.loc.column_beg == col );
        REQUIRE( lit.tok.loc.column_end == (col + 1) );
      }
      ++i;
      col += 2;
    }

  }

  SECTION( "no trailing zero" ) {

    stream_lookup.write_test("000123 2");
    auto w = reader::read("TESTSTREAM");

    REQUIRE( w.size() == 1 );
    for(auto& v : w)
    {
      REQUIRE( std::holds_alternative<literal>(v) );

      if(std::holds_alternative<literal>(v))
      {
        auto& lit = std::get<literal>(v);

        REQUIRE( lit.tok.kind == token_kind::LiteralNumber );
        REQUIRE( lit.tok.data.get_string() == std::to_string(2) );

        REQUIRE( lit.tok.loc.module == "TESTSTREAM" );
        REQUIRE( lit.tok.loc.row_beg == 1 );
        REQUIRE( lit.tok.loc.row_end == 2 );
        REQUIRE( lit.tok.loc.column_beg == 8 );
        REQUIRE( lit.tok.loc.column_end == 9 );
      }
    }
    const auto& diag = diagnostic.get_all();

    REQUIRE( diag.size() == 1);
    REQUIRE( diag[0].data.level == diag_level::error );
    REQUIRE( diag[0].data.human_referrable_code == "PA-LZ-000" );
  }
}

