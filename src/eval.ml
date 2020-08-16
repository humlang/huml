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
  | TypeAnnot_e(e,_) -> eval ctx e
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
     | Lam_e(x,b) -> eval ctx (substitute e2 x b) (* <- using e2 here means call by name *)
     | LamWithAnnot_e(x,_,b) -> eval ctx (substitute e2 x b) (* <- using e2 here means call by name *)
     | Var_e x ->
       if find_datactor x <> Option.None || find_typector x <> Option.None then
         App_e(v1, eval ctx e2)
       else
         raise Type_error
     | _ -> raise Type_error)
  | Lam_e(x,e) -> Lam_e(x,e)
  | LamWithAnnot_e(x,t,e) -> LamWithAnnot_e(x,t,e)
  | Var_e x ->
    begin
      match find_datactor x, find_typector x with
      | Option.None,Option.None ->
        begin
          match lookup_val ctx x with
          | Option.None -> raise (Unbound_variable x)
          | Option.Some e' -> e'
        end
      | Option.Some _,Option.None -> Var_e x
      | Option.None,Option.Some _ -> Var_e x
      | _,_ -> raise (Unbound_variable x)
    end
  | If_e(c,e1,e2) ->
    let c' = eval ctx c in
    (match c' with
     | Int_e 0 -> eval ctx e2
     | Int_e _ -> eval ctx e1
     | _ -> raise Type_error)
  | Match_e(e,es) ->
    let ev = eval ctx e in
    let ctx_cell = ref ctx in
    let result = List.find_opt
        (fun (p,_) ->
        let rec inner (p':pattern) (e':exp) : bool =
          begin
            match p',e' with
            | Int_p x,Int_e y -> if x = y then true else false
            | Int_p _,_ -> raise Match_error
            | Var_p x,y ->
              if Ast.find_datactor x <> Option.None then
                (match y with
                | Var_e y' -> if x = y' then true else false
                | _ -> false)
              else
                begin
                  ctx_cell := ((x,y) :: !ctx_cell);
                  true
                end
            | App_p (p1,p2),App_e(e1,e2) ->
              (match p1,e1 with
              | Var_p x,Var_e y ->
                if x = y then
                  inner p2 e2
                else
                  false
              | _,_ -> raise Match_error)
            | App_p _,_ -> false
            | Ignore_p,_ -> true
          end
        in
        inner p ev
      ) es
    in
    match result with
    | Option.None -> raise Match_error
    | Option.Some (_,e') -> eval !ctx_cell e'


(** prints constructors *)
let print_constructors (_:unit) : unit =
  let printer = HuML.ConstructorSet.iter (fun (s,e) -> Printf.printf "%s -> " s; print_exp e)
  in
  printer !HuML.Evalcontext.typectors;
  printer !HuML.Evalcontext.datactors


(** runs a program *)
let run (p:program) : Evalcontext.t =
  List.fold_left (fun ctx -> fun x ->
      match x with
      | Expr_s x' ->
        let _ = eval ctx x' in
        ctx
      | Assign_s(s,e) ->
        let v = eval ctx e in
        (s,v) :: ctx
      | Type_s(s,e) ->
        let v = eval [] e in
        HuML.Evalcontext.typectors := HuML.ConstructorSet.union (!HuML.Evalcontext.typectors)
            (HuML.ConstructorSet.singleton (s,v));
        ctx
      | Data_s(s,e) ->
        let v = eval [] e in
        HuML.Evalcontext.datactors := HuML.ConstructorSet.union (!HuML.Evalcontext.datactors)
            (HuML.ConstructorSet.singleton (s,v));
        ctx
    ) [] p
