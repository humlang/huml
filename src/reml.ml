
(* the name of the file which contains the expressions *)

let print_list = List.iter (fun a -> let _ = Ast.print_stmt a in Printf.printf "\n")

let maybe_read_line () =
  try Some(read_line())
  with End_of_file -> None

let rec input_loop acc =
  match maybe_read_line () with
  | Some(line) -> input_loop (acc @ [line])
  | None -> String.concat "\n" acc

let main () =
  let input = Lexing.from_string (input_loop []) in
  try
    let prog = Parser.prog Lexer.token input in
    Printf.printf "\nInput\n-----\n";
    print_list prog;
    Printf.printf "\nContext\n-------\n";
    Ast.print_ctx (Eval.run prog);
    Printf.printf "\n"
  with
  | Lexer.Error msg ->
      Printf.eprintf "%s%!" msg
  | Parser.Error ->
      Printf.eprintf "At offset %d: syntax error.\n%!" (Lexing.lexeme_start input)

let _ = main ()