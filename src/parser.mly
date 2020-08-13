%token <int> INT
%token TYPE_VAL

%token TYPE DATA

%token <Ast.HuML.var> IDENTIFIER

%token PLUS MINUS STAR SLASH
%token LESS LESSEQUAL GREATEREQUAL GREATER
%token LPAREN RPAREN LBRACKET RBRACKET PIPE
%token EQUAL SEMICOLON EOF EQUALARROW
%token LET BACKSLASH DOT COLON
%token IF THEN ELSE MATCH UNDERSCORE
%token APP

%type <Ast.HuML.pattern * Ast.HuML.exp> match_arm
%type <Ast.HuML.exp> expr
%type <Ast.HuML.stmt> statement
%start <Ast.HuML.stmt list> prog

%nonassoc SEMICOLON DOT ELSE
%left PLUS MINUS
%left STAR SLASH
%left LESS LESSEQUAL GREATER GREATEREQUAL
%left COLON

%nonassoc INT TYPE_VAL IDENTIFIER LPAREN LET BACKSLASH IF
%nonassoc MATCH UNDERSCORE
%nonassoc APP

%%

prog:
  | xs = list(terminated(statement, SEMICOLON)) EOF
{ xs }

statement:
  | e = expr
    { Ast.HuML.Expr_s e }
  | x = IDENTIFIER EQUAL e = expr
    { Ast.HuML.Assign_s(x,e) }
  | TYPE x = IDENTIFIER COLON e = expr
    { Ast.HuML.Type_s(x,e) }
  | DATA x = IDENTIFIER COLON e = expr
    { Ast.HuML.Data_s(x,e) }

expr:
  | i = INT
    { Ast.HuML.Int_e i }
  | TYPE_VAL
    { Ast.HuML.Type_e }
  | id = IDENTIFIER
    { Ast.HuML.Var_e id }
  | LPAREN e = expr RPAREN
    { e }
  | e = expr COLON t = expr
    { Ast.HuML.TypeAnnot_e(e,t) }
  | IF c = expr THEN e1 = expr ELSE e2 = expr
    { Ast.HuML.If_e(c, e1, e2) }
  | e1 = expr e2 = expr
    { Ast.HuML.App_e(e1,e2) } %prec APP
  | BACKSLASH x = IDENTIFIER DOT e = expr
    { Ast.HuML.Lam_e(x,e) }
  | BACKSLASH x = IDENTIFIER COLON t = expr DOT e = expr
    { Ast.HuML.LamWithAnnot_e(x,t,e) }
  | LET x = IDENTIFIER EQUAL e1 = expr SEMICOLON e2 = expr
    { Ast.HuML.Let_e(x,e1,e2) }
  | MATCH e = expr LBRACKET es = separated_list(PIPE, match_arm) RBRACKET
    { Ast.HuML.Match_e(e,es) }
  | e1 = expr PLUS e2 = expr
    { Ast.HuML.Op_e(e1,Ast.HuML.Plus,e2) }
  | e1 = expr MINUS e2 = expr
    { Ast.HuML.Op_e(e1,Ast.HuML.Minus,e2) }
  | e1 = expr STAR e2 = expr
    { Ast.HuML.Op_e(e1,Ast.HuML.Mult,e2) }
  | e1 = expr SLASH e2 = expr
    { Ast.HuML.Op_e(e1,Ast.HuML.Div,e2) }
  | e1 = expr GREATER e2 = expr
    { Ast.HuML.Op_e(e1,Ast.HuML.Gt,e2) }
  | e1 = expr GREATEREQUAL e2 = expr
    { Ast.HuML.Op_e(e1,Ast.HuML.GtEq,e2) }
  | e1 = expr LESSEQUAL e2 = expr
    { Ast.HuML.Op_e(e1,Ast.HuML.LeEq,e2) }
  | e1 = expr LESS e2 = expr
    { Ast.HuML.Op_e(e1,Ast.HuML.Le,e2) }


(* TODO: Allow underscores and such stuff *)
pattern:
  | i = INT
    { Ast.HuML.Int_p i }
  | id = IDENTIFIER
    { Ast.HuML.Var_p id }
  | UNDERSCORE
    { Ast.HuML.Ignore_p }
  | p1 = pattern p2 = pattern
    { Ast.HuML.App_p(p1,p2) } %prec APP

match_arm:
  | p = pattern EQUALARROW e = expr
    { (p,e) }
