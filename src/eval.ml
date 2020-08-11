open Ast
open Ast.HuML

exception Match_error

(** evaluates a binary operation *)
let eval_op (v1:exp) (op':op) (v2:exp) : exp =
  match v1,op',v2 with
  | Int_e i, Plus, Int_e j -> Int_e (i+j)
  | Int_e i, Minus, Int_e j -> Int_e (i-j)
  | Int_e i, Mult, Int_e j -> Int_e (i*j)
  | Int_e i, Div, Int_e j -> Int_e (i/j)
  | Int_e i, Le, Int_e j -> Int_e (if i<j then 1 else 0)
  | Int_e i, LeEq, Int_e j -> Int_e (if i<=j then 1 else 0)
  | Int_e i, GtEq, Int_e j -> Int_e (if i>=j then 1 else 0)
  | Int_e i, Gt, Int_e j -> Int_e (if i>j then 1 else 0)
  | _,_,_ -> if is_value v1 && is_value v2 then
               raise Type_error
             else
               raise Not_a_value

(** s an expression *)
let rec eval (ctx:Evalcontext.t) (e:exp) : exp =
  match e with
  | Int_e i -> Int_e i
  | Type_e -> Type_e
  | Op_e(e1,op,e2) ->
      let v1 = eval ctx e1 in
      let v2 = eval ctx e2 in
      eval_op v1 op v2
  | Let_e(x,e1,e2) ->
      let v1 = eval ctx e1 in
      let e2' = substitute v1 x e2 in
      eval ctx e2'
  | App_e(e1,e2) ->
    let v1 = eval ctx e1 in
    (*let v2 = eval ctx e2 in (* <- call by value! *)*)
    (match v1 with
     | Lam_e(x,b) -> let res = eval ctx (substitute e2 x b) in (* <- using e2 here means call by name *)
       res
     | _ -> raise Type_error)
  | Lam_e(x,e) -> Lam_e(x,e)
  | Var_e x -> lookup_val ctx x
  | If_e(c,e1,e2) ->
    let c' = eval ctx c in
    (match c' with
     | Int_e 0 -> eval ctx e2
     | Int_e _ -> eval ctx e1
     | _ -> raise Type_error)
  | Match_e(e,es) ->
    let ev = eval ctx e in
    let ctx_cell = ref ctx in
    let result = List.find_opt (fun (p,_) ->
        match p,ev with
        | Int_p x,Int_e y -> if x = y then true else false
        | Int_p _,_ -> raise Match_error
        | Var_p x,_ -> ctx_cell := Evalcontext.Value_c(x, ev, !ctx_cell); true
        | App_p _,_ -> false (* TODO: Implement this *)
        | Ignore_p,_ -> true
      ) es
    in
    match result with
    | Option.None -> raise Match_error
    | Option.Some (_,e') -> eval !ctx_cell e'

(** runs a program *)
let run (p:program) : Evalcontext.t =
  Evalcontext.constructors := Ast.collect_constructors p;
  List.fold_left (fun ctx -> fun x ->
      match x with
      | Expr_s x' ->
        let _ = eval ctx x' in
        ctx
      | Assign_s(s,e) ->
        let v = eval ctx e in
        Evalcontext.Value_c(s,v,ctx)
      | Type_s(s,e) ->
        let v = eval ctx e in
        Evalcontext.Type_c(s,v,ctx)
      | Data_s(s,e) ->
        let v = eval ctx e in
        Evalcontext.Data_c(s,v,ctx)
    ) Evalcontext.Empty_c p
