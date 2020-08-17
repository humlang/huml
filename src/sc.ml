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
let rec reduce (ctx:Evalcontext.t) (s : state) : state * Evalcontext.t =
  let (e,h) = s in
  match e with
  | Int_e i -> (Int_e i, h), ctx
  | Type_e -> (Type_e, h), ctx
  | TypeAnnot_e(e,t) -> reduce ctx (e, TypeAnnot_h(h, t))
  | Op_e(e1,op,e2) ->
    let v1 = state_to_exp (let x,_ = reduce ctx (e1,h) in x) in
    let v2 = state_to_exp (let x,_ = reduce ctx (e2,h) in x) in
    reduce_op v1 op v2 h, ctx
  | Let_e(x,e1,e2) -> (Let_e(x,e1,e2), h),ctx (* TODO *)
  | App_e(e1,e2) ->
    let v1 = state_to_exp (let x,_ = reduce ctx (e1,Hole_h) in x) in
    begin
      match v1 with
      | Lam_e(x,b) -> reduce ctx ((substitute e2 x b), h)
      | LamWithAnnot_e(x,_,b) -> reduce ctx ((substitute e2 x b), h)
      | Var_e x ->
        if find_datactor x <> Option.None || find_typector x <> Option.None then
          (App_e (v1, e2), h),ctx
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
         | Option.Some y -> y), h),ctx
      | _,_ -> (Var_e x, h),ctx
    end
  | If_e(c,e1,e2) ->
    (* TODO: We ignore the hole here, but it might be something like `$x1 <= 0`,
     *       Perhaps incorporate a SAT solver for "complex patterns", i.e. if *)
    let (c',_),_ = reduce ctx (c,Hole_h) in
    begin
      match c' with
      | Int_e 0 -> reduce ctx (e2,h)
      | Int_e _ -> reduce ctx (e1,h)
      | Var_e _ -> (If_e(c',e1,e2),h),ctx
      | _ -> raise Type_error
    end
  | Match_e(e,es) ->
    let ev = state_to_exp (let (x,_) = reduce ctx (e,Hole_h) in x) in
    if is_value ev = false then
      (Match_e(ev,es),h),ctx
    else
      let ctx_cell = ref ctx in
      let result = List.find_opt
          (fun (p,_) -> Eval.is_matching_pattern false p ev (fun (x,y) -> ctx_cell := ((x,y) :: !ctx_cell))
        ) es
      in
      match result with
      | Option.None -> raise Eval.Match_error
      | Option.Some (_,e') -> reduce !ctx_cell (e',h)


let eliminate_duplicates_from_list (lst : ('a * 'b) list) =
  let ln = List.length lst in
  let seen = Hashtbl.create ln in
  List.filter (fun (_,y) -> let tmp = not (Hashtbl.mem seen y) in
                        Hashtbl.replace seen y ();
                        tmp) lst

(** The entry point for our supercompilation *)
let sc (ctx:Evalcontext.t) (h : history) (s : state) : exp =
  let rec sc' (h' : history) (s',ctx' : state*Evalcontext.t) : state =
    match terminate h' s' with
    | Continue h'' -> sc' h'' (split h'' (reduce ctx' s')) (* TODO: change this to something more meaningful *)
    | Stop -> s'
    and split (h':history) (s,ctx':state*Evalcontext.t) : state*Evalcontext.t =
    begin
      let (e,eh) = s in
      match e with
      | If_e(c,e1,e2) ->
        let a = state_to_exp (sc' h' ((e1,eh),ctx')) in
        let b = state_to_exp (sc' h' ((e2,eh),ctx')) in
        if a <> b then
          (If_e(c,a,b), eh),ctx'
        else
          (a,eh),ctx'
        (* TODO: Unification! We need a SAT-solver here... *)
      | Match_e(c,es) ->
        begin
          Printf.printf "split for match on "; print_exp c; Printf.printf "\n";
          let us = state_to_exp (let x,_ = reduce ctx (c,Hole_h) in x) in
          let xs = eliminate_duplicates_from_list
              (List.map (fun (p, e') ->
                   let ctx_cell = ref ctx in
                   if Eval.is_matching_pattern true p c
                       (fun (x,y) -> ctx_cell := (x,y) :: !ctx_cell) then
                     p,state_to_exp (sc' h' ((e',Hole_h),!ctx_cell))
                   else
                     raise Eval.Match_error
                 ) es)
          in
            if List.length xs == 1 then
              (let _,x = List.hd xs in x, eh),ctx
            else if List.length xs == 0 then
              raise Eval.Match_error
            else
              (Match_e(us, xs), eh),ctx
        end
      | _ -> s,ctx
    end
  in
    state_to_exp (sc' h (s,ctx))
