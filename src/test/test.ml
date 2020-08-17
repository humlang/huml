
open OUnit2
open Lib

let get_prog code =
  Parser.prog Lexer.token (Lexing.from_string code)

let tests = "test suite for binary operations" >::: [
    "plus"   >:: (fun _ ->
        let _ = get_prog "5 + 3" in
        assert_equal 0 0;
      )
]

let _ = run_test_tt_main tests
