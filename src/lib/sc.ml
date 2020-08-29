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

(** Print history *)
let print_history (h : history) : unit =
  Printf.printf "BEGIN History:\n";
  List.iteri (fun i -> fun e ->
      Printf.printf "  h - %d : " i;
      print_exp (state_to_exp e);
      Printf.printf "\n";
    ) h;
  Printf.printf "END History\n"


type shell = Shell of string * (shell list)

let rec hom (x:shell) (y:shell) = dive x y || couple x y
and dive (x:shell) (Shell(_,ys):shell) = List.exists (fun z -> hom x z) ys
and couple (Shell(x,xs):shell) (Shell(y,ys):shell) =
  x = y
  && List.length xs = List.length ys
  && List.for_all2 (fun a -> fun b -> hom a b) xs ys

let rec shell (e:exp): shell =
  match e with
  | HuML.Type_e -> Shell ("#Type", [])
  | HuML.TypeAnnot_e(e',_) -> shell e'
  | HuML.Op_e(e1,op,e2) -> Shell (String.concat "#BinOp" [string_of_op op], [shell e1 ; shell e2])
  | HuML.Var_e x ->
    if find_datactor x <> Option.None || find_typector x <> Option.None then
      Shell (x, [])
    else
      Shell ("#Var", [])
  | HuML.Int_e i -> Shell (string_of_int i, [])
  | HuML.App_e(a,b) -> Shell ("#App", [shell a ; shell b])
  | HuML.Let_e(_,a,b) -> Shell ("#Let", [shell a ; shell b])
  | HuML.Lam_e(_,b) -> Shell ("#Lam", [shell b])
  | HuML.LamWithAnnot_e(_,_,b) -> Shell ("#Lam", [shell b])
  | HuML.If_e(c,a,b) -> Shell ("#If", shell c :: shell a :: [shell b])
  | HuML.Match_e(id,es) -> Shell ("#Match", shell id :: List.map (fun (_,y) -> shell y) es)

(** Homeomorphic embedding *)
let homeo (x:exp) (y:exp) : bool =
  let (Shell(sx',sx),Shell(sy',sy)) = (shell x, shell y) in
  Printf.printf "homeo    -     %s <=?=> %s     " sx' sy';
  let b = hom (Shell(sx',sx)) (Shell(sy',sy)) in
  begin
  if b then
    Printf.printf "OK\n"
  else
    Printf.printf "NK\n"
  end;
  b



(** Checks whether supercompilation should terminate here or not *)
let terminate (h : history) ((sa,sb) : state) : termination =
  print_history h;
  if List.find_opt (fun (a,_) -> homeo a sa) h <> Option.None then (* TODO: Make less conservative *)
    (Printf.printf "Terminate - STOP \n";
    Stop)
  else
    (Printf.printf "Terminate - Continue \n";
    Continue ((sa,sb) :: h))

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
  Printf.printf "Reduce : ";
  print_exp e;
  Printf.printf "\n";
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
      (*
      Printf.printf "Reduce [App] : "; print_exp v1; Printf.printf "   "; print_exp e2; Printf.printf "\n";
       *)
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
  | Match_e(id,es) ->
    let ctx_cell = ref ctx in
    let var = state_to_exp (let (n,_) = reduce ctx ((match id with
        | Var_e x -> (match lookup_val ctx x with
             | Option.Some e' -> e'
             | Option.None -> Var_e x)
        | _ -> id),Hole_h) in n) in
    let result = List.find_opt
        (fun (p,_) -> Eval.is_matching_pattern false p var (fun (x,y) -> ctx_cell := ((x,y) :: !ctx_cell))
      ) es
    in
    match result with
    | Option.None ->
      Printf.printf "03 match error  :  ";
      print_exp (Match_e(id,es));
      Printf.printf "\n";
      (Match_e(id,es),h),!ctx_cell
    | Option.Some (_,e') -> reduce !ctx_cell (e',h)


let eliminate_duplicates_from_list (lst : ('a * 'b) list) =
  let ln = List.length lst in
  let seen = Hashtbl.create ln in
  List.filter (fun (_,y) -> let tmp = not (Hashtbl.mem seen y) in
                        Hashtbl.replace seen y ();
                        tmp) lst

let rec filter_map (f:'a -> 'b option) (xs:'a list) : 'b list =
  match xs with
  | []       -> []
  | x :: xs' ->
    match f x with
    | Option.None    -> filter_map f xs'
    | Option.Some x' -> x' :: (filter_map f xs')

(** The entry point for our supercompilation *)
let sc (ctx:Evalcontext.t) (h : history) (s : state) : exp =
  let rec pat_to_exp_for_binding_exp (p:pattern) (e:exp) : exp =
    match p with
    | HuML.Int_p i -> Int_e i
    | HuML.Var_p y -> Var_e y
    | HuML.App_p(a,b) -> App_e (pat_to_exp_for_binding_exp a e, pat_to_exp_for_binding_exp b e)
    | HuML.Ignore_p -> e
  in
  let rec sc' (h' : history) (s',ctx' : state*Evalcontext.t) : state =
    match terminate h' s' with
    | Continue h'' -> let ((re,rh),rc) = reduce ctx' s'
      in
      Printf.printf "sc - "; print_exp (let (e,_) = s in e);
      Printf.printf " reduces to "; print_exp re; Printf.printf "\n";

      let ((se,sh),zc) = split h'' ((re, rh), rc) in

      Printf.printf "sc - Split : "; print_exp se; Printf.printf "\n";

      sc' h'' ((se,sh),zc) (* TODO: change this to something more meaningful *)

    | Stop -> s'
    and split (h':history) (s,ctx':state*Evalcontext.t) : state*Evalcontext.t =
    begin
      let (e,eh) = s in
      Printf.printf "sc - Split : "; print_exp (state_to_exp (e,eh)); Printf.printf "\n";
      match e with
      | If_e(c,e1,e2) ->
        let a = state_to_exp (sc' h' ((e1,eh),ctx')) in
        let b = state_to_exp (sc' h' ((e2,eh),ctx')) in
        if a <> b then
          (If_e(c,a,b), eh),ctx'
        else
          (a,eh),ctx'
        (* TODO: Unification! We need a SAT-solver here... *)
      | Match_e(matchon,es) ->
        begin
          let id = match matchon with
            | HuML.Var_e x -> x
            | _ -> raise Eval.Match_error
          in
          let c = match lookup_val ctx id with
            | Option.Some e' -> e'
            | Option.None -> Var_e id
          in
          Printf.printf "split for match on  %s = " id; print_exp c; Printf.printf "\n";
          let us = state_to_exp (let x,_ = split h' (reduce ctx (c,Hole_h)) in x) in
          Printf.printf "split for match on us = "; print_exp us; Printf.printf "\n";
          let xs = eliminate_duplicates_from_list
              (filter_map (fun (p, e') ->
                   let ctx_cell = ref ctx in
                   if Eval.is_matching_pattern true p c
                       (fun (x,y) -> ctx_cell := (x,y) :: !ctx_cell) then
                     let m = state_to_exp (sc' h' ((substitute (pat_to_exp_for_binding_exp p us) id e',Hole_h),!ctx_cell)) in
                     Option.Some (p,m)
                   else
                     begin
                       Printf.printf "02 match error: c = \"";
                       print_exp us;
                       Printf.printf "\"  p = \"";
                       print_pattern p;
                       Printf.printf "\"   e' = \"";
                       print_exp e';
                       Printf.printf "\"\n";
                       Option.None
                     end
                 ) es)
          in
            if List.length xs == 1 then
              (let _,x = List.hd xs in x, eh),ctx
            else if List.length xs == 0 then
              begin
                Printf.printf "01 match error\n";
                raise Eval.Match_error
              end
            else
              (Match_e(us,xs),eh),ctx
        end
      | _ -> s,ctx
    end
  in
    state_to_exp (sc' h (reduce ctx s))
