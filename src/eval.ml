open Ast

(** evaluates a binary operation *)
let eval_op (v1:Ast.HuML.exp) (op':Ast.HuML.op) (v2:Ast.HuML.exp) : Ast.HuML.exp =
  match v1,op',v2 with
  | Ast.HuML.Int_e i, Ast.HuML.Plus, Ast.HuML.Int_e j -> Ast.HuML.Int_e (i+j)
  | Ast.HuML.Int_e i, Ast.HuML.Minus, Ast.HuML.Int_e j -> Ast.HuML.Int_e (i-j)
  | Ast.HuML.Int_e i, Ast.HuML.Mult, Ast.HuML.Int_e j -> Ast.HuML.Int_e (i*j)
  | Ast.HuML.Int_e i, Ast.HuML.Div, Ast.HuML.Int_e j -> Ast.HuML.Int_e (i/j)
  | Ast.HuML.Int_e i, Ast.HuML.Le, Ast.HuML.Int_e j -> Ast.HuML.Int_e (if i<j then 1 else 0)
  | Ast.HuML.Int_e i, Ast.HuML.LeEq, Ast.HuML.Int_e j -> Ast.HuML.Int_e (if i<=j then 1 else 0)
  | Ast.HuML.Int_e i, Ast.HuML.GtEq, Ast.HuML.Int_e j -> Ast.HuML.Int_e (if i>=j then 1 else 0)
  | Ast.HuML.Int_e i, Ast.HuML.Gt, Ast.HuML.Int_e j -> Ast.HuML.Int_e (if i>j then 1 else 0)
  | _,_,_ -> if is_value v1 && is_value v2 then
               raise Type_error
             else
               raise Not_a_value

(** s an expression *)
let rec eval (ctx:Ast.HuML.eval_ctx) (e:Ast.HuML.exp) : Ast.HuML.exp =
  match e with
  | Ast.HuML.Int_e i -> Ast.HuML.Int_e i
  | Ast.HuML.Type_e -> Ast.HuML.Type_e
  | Ast.HuML.Op_e(e1,op,e2) ->
      let v1 = eval ctx e1 in
      let v2 = eval ctx e2 in
      eval_op v1 op v2
  | Ast.HuML.Let_e(x,e1,e2) ->
      let v1 = eval ctx e1 in
      let e2' = substitute v1 x e2 in
      eval ctx e2'
  | Ast.HuML.App_e(e1,e2) ->
    let v1 = eval ctx e1 in
    (*let v2 = eval ctx e2 in (* <- call by value! *)*)
    (match v1 with
     | Ast.HuML.Lam_e(x,b) -> let res = eval ctx (substitute e2 x b) in (* <- using e2 here means call by name *)
       res
     | _ -> raise Type_error)
  | Ast.HuML.Lam_e(x,e) -> Ast.HuML.Lam_e(x,e)
  | Ast.HuML.Var_e x -> lookup_val ctx x
  | Ast.HuML.If_e(c,e1,e2) ->
    let c' = eval ctx c in
    (match c' with
     | Ast.HuML.Int_e 0 -> eval ctx e2
     | Ast.HuML.Int_e _ -> eval ctx e1
     | _ -> raise Type_error)
  | Ast.HuML.Match_e(e,_) ->
    e (* TODO *)

(** runs a program *)
let run (p:Ast.HuML.program) : Ast.HuML.eval_ctx =
  List.fold_left (fun ctx -> fun x ->
      match x with
      | Ast.HuML.Expr_s x' ->
        let _ = eval ctx x' in
        ctx
      | Ast.HuML.Assign_s(s,e) ->
        let v = eval ctx e in
        Ast.HuML.Value_c(s,v,ctx)
      | Ast.HuML.Type_s(s,e) ->
        let v = eval ctx e in
        Ast.HuML.Type_c(s,v,ctx)
      | Ast.HuML.Data_s(s,e) ->
        let v = eval ctx e in
        Ast.HuML.Data_c(s,v,ctx)
    ) Ast.HuML.Empty_c p
