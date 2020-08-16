open Ast
open Ast.HuML


type holeexp =
  | Hole_h
  | Lam_h of var * holeexp
  | LamWithAnnot_h of var * exp * holeexp
  | TypeAnnot_h of holeexp * exp
  | LOp_h of holeexp * op * exp
  | ROp_h of exp * op * holeexp

type state = exp * holeexp
type history = state list

type termination =
  | Stop
  | Continue of history


let empty_state (e : exp) : state =
  (e, Hole_h)

let state_to_exp (s : state) : exp =
  let rec fill_hole (h : holeexp) (e : exp) : exp =
    match h with
    | Hole_h -> e
    | Lam_h (x,h') -> fill_hole h' (Lam_e(x, e))
    | LamWithAnnot_h (x,t,h') -> fill_hole h' (LamWithAnnot_e(x,t,e))
    | TypeAnnot_h (h',t) -> fill_hole h' (TypeAnnot_e(e,t))
    | LOp_h (h',op,e') -> fill_hole h' (Op_e(e,op,e'))
    | ROp_h (e',op,h') -> fill_hole h' (Op_e(e',op,e))
  in
  let (e, h) = s
  in
    fill_hole h e

(** Checks whether supercompilation should terminate here or not *)
let terminate (h : history) (s : state) : termination =
  (* TODO: This equality check is wrong, we actually want the homeomorphic embedding here!! *)
  if List.find_opt (fun a -> a = s) h <> Option.None then (* TODO: Make less conservative *)
    Stop
  else
    Continue (s :: h)

(** evaluates a binary operation *)
let reduce_op (v1:exp) (op':op) (v2:exp) (h:holeexp) : state =
  match v1,op',v2 with
  | Int_e i, Plus, Int_e j -> (Int_e (i+j), h)
  | Int_e i, Minus, Int_e j -> (Int_e (i-j), h)
  | Int_e i, Mult, Int_e j -> (Int_e (i*j), h)
  | Int_e i, Div, Int_e j -> (Int_e (i/j), h)
  | Int_e i, Le, Int_e j -> (Int_e (if i<j then 1 else 0), h)
  | Int_e i, LeEq, Int_e j -> (Int_e (if i<=j then 1 else 0), h)
  | Int_e i, GtEq, Int_e j -> (Int_e (if i>=j then 1 else 0), h)
  | Int_e i, Gt, Int_e j -> (Int_e (if i>j then 1 else 0), h)
  | Int_e _, _, _ -> (v2, ROp_h(v1, op', h))
  | _, _, Int_e _ -> (v1, LOp_h(h, op', v2))
  | _,_,_ -> (v1, LOp_h(h, op', v2)) (* TODO: this is nondeterministically chosen. we want to actually analyze both paths! *)

(** This partially evaluates until one cannot proceed any further *)
let rec reduce (ctx:Evalcontext.t) (s : state) : state =
  let (e,h) = s in
  match e with
  | Int_e i -> (Int_e i, h)
  | Type_e -> (Type_e, h)
  | TypeAnnot_e(e,t) -> reduce ctx (e, TypeAnnot_h(h, t))
  | Op_e(e1,op,e2) ->
    let v1 = state_to_exp (reduce ctx (e1,h)) in
    let v2 = state_to_exp (reduce ctx (e2,h)) in
    reduce_op v1 op v2 h
  | Let_e(x,e1,e2) -> (Let_e(x,e1,e2), h) (* TODO *)
  | App_e(e1,e2) ->
    let v1 = state_to_exp (reduce ctx (e1,Hole_h)) in
    begin
      match v1 with
      | Lam_e(x,b) -> reduce ctx ((substitute e2 x b), h)
      | LamWithAnnot_e(x,_,b) -> reduce ctx ((substitute e2 x b), h)
      | Var_e x ->
        if find_datactor x <> Option.None || find_typector x <> Option.None then
          (App_e (v1, e2), h)
        else
          raise Type_error  (* If we don't know the function, we also cannot supercompile it *)
      | _ -> raise Type_error
    end
  | Lam_e(x,e) -> reduce ctx (e, Lam_h(x, h))
  | LamWithAnnot_e(x,t,e) -> reduce ctx (e, LamWithAnnot_h(x,t,h))
  | Var_e x ->
    begin
      match find_datactor x, find_typector x with
      | Option.None,Option.None ->
        ((match lookup_val ctx x with
         | Option.None -> Var_e x
         | Option.Some y -> y), h)
      | _,_ -> (Var_e x, h)
    end
  | If_e(c,e1,e2) -> (If_e(c,e1,e2), h) (* TODO *)
  | Match_e(e,es) ->
    let ev = state_to_exp (reduce ctx (e,Hole_h)) in
    if is_value ev = false then
      (Match_e(ev,es),h)
    else
      let ctx_cell = ref ctx in
      let result = List.find_opt
          (fun (p,_) ->
          let rec inner (p':pattern) (e':exp) : bool =
            begin
              match p',e' with
              | Int_p x,Int_e y -> if x = y then true else false
              | Int_p _,_ -> raise Eval.Match_error
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
                | _,_ -> raise Eval.Match_error)
              | App_p _,_ -> false
              | Ignore_p,_ -> true
            end
          in
          inner p ev
        ) es
      in
      match result with
      | Option.None -> raise Eval.Match_error
      | Option.Some (_,e') -> reduce !ctx_cell (e',h)

(** The entry point for our supercompilation *)
let sc (ctx:Evalcontext.t) (h : history) (s : state) : exp =
  let rec sc' (h' : history) (s' : state) : exp =
    match terminate h' s' with
    | Continue h'' -> sc' h'' (reduce ctx s') (* TODO: change this to something more meaningful *)
    | Stop -> state_to_exp s'
  in
    sc' h s
