{
    open Parser

    exception Error of string
}

rule token = parse
  | [' ' '\t' '\n'] { token lexbuf }
  | ';' { SEMICOLON }
  | '0' | ['1'-'9']['0'-'9']* as i { INT (int_of_string i) }
  | "type" { TYPE }
  | "Type" { TYPE_VAL }
  | "data" { DATA }
  | "if" { IF }
  | "then" { THEN }
  | "else" { ELSE }
  | "match" { MATCH }
  | "let" { LET }
  | ['a'-'z''A'-'Z''_']['a'-'z''A'-'Z''0'-'9''_']* as i { IDENTIFIER i }
  | '<' { LESS }
  | "<=" { LESSEQUAL }
  | ">=" { GREATEREQUAL }
  | '>' { GREATER }
  | '\\' { BACKSLASH }
  | "->" { ARROW }
  | "=>" { EQUALARROW }
  | '.' { DOT }
  | ':' { COLON }
  | '+' { PLUS }
  | '-' { MINUS }
  | '/' { SLASH }
  | '*' { STAR }
  | '(' { LPAREN }
  | ')' { RPAREN }
  | '[' { LBRACKET }
  | ']' { RBRACKET }
  | '|' { PIPE }
  | '_' { UNDERSCORE }
  | '=' { EQUAL }
  | eof { EOF }
  | _ { raise (Error (Printf.sprintf "%d: unexpected character.\n" (Lexing.lexeme_start lexbuf))) }
