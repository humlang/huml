
open OUnit2
open Lib

let get_prog code =
  Parser.prog Lexer.token (Lexing.from_string code)

let get_supercompiled_results ctx prog =
  List.fold_left (fun xs -> fun a ->
      match a with
      | Ast.HuML.Expr_s e ->
        begin
          let res = Sc.sc ctx [] (Sc.empty_state e) in
          res :: xs
        end
      | _ -> xs
    ) [] prog


let test_simple_add_if _ =
  let p = get_prog
{|
nadd = \s. \x. \y. if x <= 0 then (if y <= 0 then 0 else y) else (if y <= 0 then x else s (x-1) (y - 1));
Y = \f. (\x. f (x x)) (\x. f (x x));
add = \x. \y. Y nadd x y;
add 1 1;
|}
  in
  let scs = get_supercompiled_results (Eval.run p) p in
  assert_equal (List.length scs) 1;
  let hd = List.hd scs in
  assert_equal hd (Ast.HuML.Int_e 0)

let test_simple_add_match _ =
  let p = get_prog
{|
nadd = \s. \x. \y. match x [ 0 => match y [ 0 => 0 | _ => y ] | _ => match y [ 0 => x | _ => s (x - 1) (y - 1) ] ];
Y = \f. (\x. f (x x)) (\x. f (x x));
add = \x. \y. Y nadd x y;
add 1 1;
|}
  in
  let scs = get_supercompiled_results (Eval.run p) p in
  assert_equal (List.length scs) 1;
  let hd = List.hd scs in
  assert_equal hd (Ast.HuML.Int_e 0)


let test_simple_match_on_nat _ =
  let p = get_prog
{|
type Nat : Type;
data Z : Nat;
data S : Nat -> Nat;

\x. match x [ Z => Z | S y => Z ];
|}
  in
  let scs = get_supercompiled_results (Eval.run p) p in
  assert_equal (List.length scs) 1;
  let hd = List.hd scs in
  assert_equal hd (Ast.HuML.Lam_e("x", Ast.HuML.Var_e "Z"))


let test_match_elim _ =
  let p = get_prog
{|
type Nat : Type;
data Z : Nat;
data S : Nat -> Nat;

foo = \n. match n [Z => S Z | S Z => S(S Z) | S(S Z) => Z | S x => S(S(S Z))];

fooZ = foo (S(S Z));
fooSZ = foo Z;
fooSSZ = foo (S Z);
fooSSSZ = foo (S(S(S Z)));

\n. foo Z;
\n. foo (S n);
\n. foo (S(S n));
|}
  in
  let scs = get_supercompiled_results (Eval.run p) p in
  assert_equal (List.length scs) 3;
  let gold =
    List.map (fun a ->
        match a with
        | Ast.HuML.Expr_s e -> e
        | _ -> raise Ast.Type_error
      )
      (get_prog "\\n. S Z; \\n. match n [S Z => S(S Z) | S(S Z) => Z | S x => S(S(S Z))]; \\n. match (S (S n)) [S S Z => Z | S x => (S (S (S Z)))];")
  in
  assert_equal scs gold


let tests = "test suite for supercompilation" >::: [
    "simple_add_if"               >:: test_simple_add_if ;
    "simple_add_match"            >:: test_simple_add_match ;
    "simple_match_on_nat"         >:: test_simple_match_on_nat ;
    "match_elim"                  >:: test_match_elim
]

let _ = run_test_tt_main tests
