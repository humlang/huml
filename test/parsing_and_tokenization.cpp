#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <stream_lookup.hpp>
#include <diagnostic.hpp>
#include <reader.hpp>

#include <sstream>

TEST_CASE( "Assignments are parsed correctly", "[Assignments]" ) {

    SECTION( "assignment_with_whitespace" ) {
      diagnostic.reset();
      stream_lookup.write_test("x := 2;");
      auto w = hx_reader::read<ast_type>("TESTSTREAM");

      std::vector<std::uint_fast64_t> ids;

      REQUIRE(w.size() == 1);
      const bool is_assignstmt = std::holds_alternative<stmt_type>(w[0]);
      REQUIRE(is_assignstmt);
      if(!is_assignstmt)
        return; // bail out, we cannot go on!
      auto& assstmtmaybe = std::get<stmt_type>(w[0]);
      const bool is_assign = std::holds_alternative<assign>(assstmtmaybe);
      REQUIRE(is_assign);
      if(!is_assign)
        return; // bail out, we cannot go on!
      auto& ass = std::get<assign>(assstmtmaybe);
      ids.push_back(ass->id());

      REQUIRE(ass->tok.kind == token_kind::Assign);
      REQUIRE(ass->tok.data.get_string() == ":=" );

      auto& id = std::get<identifier>(ass->var());
      REQUIRE(id->tok.kind == token_kind::Identifier );
      REQUIRE(id->tok.data.get_string() == "x" );
      ids.push_back(id->id());

      auto& exp = ass->exp();
      const bool is_litexpr = std::holds_alternative<exp_type>(exp);
      REQUIRE(is_litexpr);
      if(!is_litexpr)
        return; // bail out, we cannot go on!
      auto& litexpr = std::get<exp_type>(exp);
      const bool is_lit = std::holds_alternative<literal>(litexpr);
      REQUIRE(is_lit);
      if(!is_lit)
        return; // bail out, we cannot go on!
      auto& lit = std::get<literal>(litexpr);
      REQUIRE(lit->tok.kind == token_kind::LiteralNumber );
      REQUIRE(lit->tok.data.get_string() == "2" );
      ids.push_back(lit->id());

      // we want that each node has a unique id
      std::sort(ids.begin(), ids.end());
      REQUIRE((std::adjacent_find(ids.begin(), ids.end()) == ids.end()));
    }

    SECTION( "assignment-without-whitespace" ) {
      diagnostic.reset();
      stream_lookup.write_test("x:=2;");
      auto w = hx_reader::read<ast_type>("TESTSTREAM");

      REQUIRE(w.size() == 1);
      const bool is_assignstmt = std::holds_alternative<stmt_type>(w[0]);
      REQUIRE(is_assignstmt);
      if(!is_assignstmt)
        return; // bail out, we cannot go on!
      auto& assstmt = std::get<stmt_type>(w[0]);
      const bool is_assign = std::holds_alternative<assign>(assstmt);
      REQUIRE(is_assign);
      if(!is_assign)
        return; // bail out, we cannot go on!
      auto& ass = std::get<assign>(assstmt);

      REQUIRE(ass->tok.kind == token_kind::Assign);
      REQUIRE(ass->tok.data.get_string() == ":=" );

      auto& id = std::get<identifier>(ass->var());
      REQUIRE(id->tok.kind == token_kind::Identifier );
      REQUIRE(id->tok.data.get_string() == "x" );

      auto& exp = ass->exp();
      const bool is_litexpr = std::holds_alternative<exp_type>(exp);
      REQUIRE(is_litexpr);
      if(!is_litexpr)
        return; // bail out, we cannot go on!
      auto& litexpr = std::get<exp_type>(exp);
      const bool is_lit = std::holds_alternative<literal>(litexpr);
      REQUIRE(is_lit);
      if(!is_lit)
        return; // bail out, we cannot go on!
      auto& lit = std::get<literal>(litexpr);
      REQUIRE(lit->tok.kind == token_kind::LiteralNumber );
      REQUIRE(lit->tok.data.get_string() == "2" );
    }
}

TEST_CASE( "Expression test parsing", "[Expressions]" ) {

  SECTION( "expression-with-whitespace" ) {
    diagnostic.reset();
    stream_lookup.write_test("x := 6*7;");
    auto w = hx_reader::read<ast_type>("TESTSTREAM");
    
    REQUIRE(w.size() == 1);
    const bool is_assign_stmt = std::holds_alternative<stmt_type>(w[0]);
    if(!is_assign_stmt)
      return; 
    REQUIRE(is_assign_stmt);
    auto& assstmt = std::get<stmt_type>(w[0]);
    const bool is_assign = std::holds_alternative<assign>(assstmt);
    REQUIRE(is_assign);
    if(!is_assign)
      return; 
    
    auto& ass = std::get<assign>(assstmt);
    auto& exp = ass->exp();
    const bool is_binexpex = std::holds_alternative<exp_type>(exp);
    REQUIRE(is_binexpex);
    if(!is_binexpex)
      return;
    auto& bin_expex = std::get<exp_type>(exp);
    const bool is_binexp = std::holds_alternative<binary_exp>(bin_expex);
    REQUIRE(is_binexp);
    if(!is_binexp)
      return;
    auto& bin_exp = std::get<binary_exp>(bin_expex);
    
    REQUIRE(bin_exp->tok.kind == token_kind::Asterisk);
    REQUIRE(bin_exp->tok.data.get_string() == "*" );
    
    auto& left_exp = bin_exp->get_left_exp();
    bool left_is_idex = std::holds_alternative<exp_type>(left_exp);
    REQUIRE(left_is_idex);
    if(!left_is_idex)
      return;
    
    auto& leftex = std::get<exp_type>(left_exp);
    bool left_is_id = std::holds_alternative<literal>(leftex);
    REQUIRE(left_is_id);
    if(!left_is_id)
      return;
    
    auto& left = std::get<literal>(leftex);
    REQUIRE(left->tok.kind == token_kind::LiteralNumber);
    REQUIRE(left->tok.data.get_string() == "6" );


    auto& right_exp = bin_exp->get_right_exp();
    bool right_is_idex = std::holds_alternative<exp_type>(right_exp);
    REQUIRE(right_is_idex);
    if(!right_is_idex)
      return;
    
    auto& rightex = std::get<exp_type>(right_exp);
    bool right_is_id = std::holds_alternative<literal>(rightex);
    REQUIRE(right_is_id);
    if(!right_is_id)
      return;
    
    auto& right = std::get<literal>(rightex);
    REQUIRE(right->tok.kind == token_kind::LiteralNumber);
    REQUIRE(right->tok.data.get_string() == "7" );
  }
}

