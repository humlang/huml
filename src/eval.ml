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
        let rec inner (p':pattern) (e':exp) : bool =
          match p',e' with
          | Int_p x,Int_e y -> if x = y then true else false
          | Int_p _,_ -> raise Match_error
          | Var_p x,y ->
            if Ast.find_datactor x then
              (match y with
               | Var_e y' -> if x = y' then true else false
               | _ -> raise Match_error)
            else
              (ctx_cell := ((x,ev) :: !ctx_cell); true)
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
in
inner p ev
      ) es
    in
    match result with
    | Option.None -> raise Match_error
    | Option.Some (_,e') -> eval !ctx_cell e'



(** collects all constructors occuring in the program *)
let collect_constructors (p:HuML.program) : unit =
  let rec collect (ct:HuML.ConstructorSet.t) (cd:HuML.ConstructorSet.t) (p':HuML.program)
    : HuML.ConstructorSet.t * HuML.ConstructorSet.t =
    match p' with
    | [] -> (ct,cd)
    | (HuML.Expr_s _ | HuML.Assign_s _) :: xs -> collect ct cd xs
    | HuML.Type_s(v,e) :: xs ->
      let e' = eval [] e in
      let (ct',cd') = collect (HuML.ConstructorSet.union ct (HuML.ConstructorSet.singleton (v,e'))) cd xs
      in
        (ct',cd')
    | HuML.Data_s(v,e) :: xs ->
      let e' = eval [] e in
      let (ct',cd') = collect ct (HuML.ConstructorSet.union cd (HuML.ConstructorSet.singleton (v,e'))) xs
      in
        (ct',cd')
  in
    let (ct, cd) = collect HuML.ConstructorSet.empty HuML.ConstructorSet.empty p
  in
    HuML.Evalcontext.typectors := ct;
    HuML.Evalcontext.datactors := cd;
    ()


(** runs a program *)
let run (p:program) : Evalcontext.t =
  collect_constructors p;
  List.fold_left (fun ctx -> fun x ->
      match x with
      | Expr_s x' ->
        let _ = eval ctx x' in
        ctx
      | Assign_s(s,e) ->
        let v = eval ctx e in
        (s,v) :: ctx
      | (Type_s _ | Data_s _) -> ctx (* HACK: collect_constructors already deals with those *)
                                     (* TODO: Types might be declared later but known earlier *)
    ) [] p
