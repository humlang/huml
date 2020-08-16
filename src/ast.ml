
module HuML = struct
  type var = string

  module VarSet = Set.Make(String)

  let gensym = let counter = ref 0 in
                fun () ->
                  incr counter; "$x" ^ string_of_int !counter


  type op = Plus | Minus | Mult | Div
          | Le   | LeEq  | Gt   | GtEq

  type pattern =
    | Int_p of int
    | Var_p of var
    | App_p of pattern * pattern
    | Ignore_p
  and exp =
    | TypeAnnot_e of exp * exp
    | Int_e of int
    | Type_e
    | Op_e of exp * op * exp
    | Var_e of var
    | Let_e of var * exp * exp
    | Lam_e of var * exp
    | LamWithAnnot_e of var * exp * exp
    | App_e of exp * exp
    | If_e of exp * exp * exp
    | Match_e of exp * ((pattern * exp) list)

  type stmt =
    | Expr_s of exp
    | Assign_s of var * exp
    | Type_s of var * exp
    | Data_s of var * exp

  module Constructor = struct
    type t = var * exp

    let compare = compare
  end
  module ConstructorSet = Set.Make(Constructor)

  type program = stmt list

  module Evalcontext = struct
    type t = (var * exp) list

    let typectors = ref ConstructorSet.empty
    let datactors = ref ConstructorSet.empty
  end

end

exception Unbound_variable of HuML.var
exception Type_error
exception Not_a_value

(** looksup a variable in the evaluation context *)
let lookup_val (ctx:HuML.Evalcontext.t) (s:HuML.var) : HuML.exp option =
  let rec lookup_val' (ctx':HuML.Evalcontext.t) : HuML.exp option =
    match ctx' with
    | [] -> Option.None
    | (v,e) :: xs -> if s = v then Option.Some e else lookup_val' xs
  in
    lookup_val' ctx

(** finds a data ctor in the evaluation context *)
let find_datactor (s:HuML.var) : HuML.exp option =
  let w = List.find_opt
      (fun (z,_) -> if s = z then true else false)
      (HuML.ConstructorSet.elements !HuML.Evalcontext.datactors)
  in
  match w with
  | Option.None -> Option.None
  | Option.Some(_,v) -> Option.Some v

(** finds a type ctor in the evaluation context *)
let find_typector (s:HuML.var) : HuML.exp option =
  let w = List.find_opt
      (fun (z,_) -> if s = z then true else false)
      (HuML.ConstructorSet.elements !HuML.Evalcontext.typectors)
  in
  match w with
  | Option.None -> Option.None
  | Option.Some(_,v) -> Option.Some v

(** [fv e] is a set-like list of the free variables of [e]. *)
let rec fv (e:HuML.exp) : HuML.VarSet.t =
  match e with
  | HuML.(Type_e | HuML.Int_e _) -> HuML.VarSet.empty
  | HuML.TypeAnnot_e(e',t) -> HuML.VarSet.union (fv e') (fv t)
  | HuML.Var_e x -> HuML.VarSet.singleton x
  | HuML.App_e(e1, e2) -> HuML.VarSet.union (fv e1) (fv e2)
  | HuML.Lam_e(x,e') -> HuML.VarSet.diff (fv e') (HuML.VarSet.singleton x)
  | HuML.LamWithAnnot_e(x,t,e') -> HuML.VarSet.union (fv t) (HuML.VarSet.diff (fv e') (HuML.VarSet.singleton x))
  | HuML.Op_e(e1,_,e2) -> HuML.VarSet.union (fv e1) (fv e2)
  | HuML.Let_e(z,e1,e2) -> HuML.VarSet.union (fv e1) (HuML.VarSet.diff (fv e2) (HuML.VarSet.singleton z))
  | HuML.If_e(c,e1,e2) -> HuML.VarSet.union (fv c) (HuML.VarSet.union (fv e1) (fv e2))
  | HuML.Match_e(e, es) -> HuML.VarSet.union (fv e) (List.fold_left (fun a -> fun (_,e) ->
      HuML.VarSet.union a (fv e)) HuML.VarSet.empty es)


(** reames x to y in expression e *)
let rec rename (x:HuML.var) (y:HuML.var) (e:HuML.exp) : HuML.exp =
  let rec rename' (p:HuML.pattern) : HuML.pattern =
    match p with
    | HuML.Int_p i -> HuML.Int_p(i)
    | HuML.App_p(e1,e2) -> HuML.App_p(rename' e1, rename' e2)
    | HuML.Var_p z -> if z = x then HuML.Var_p z else HuML.Var_p y
    | HuML.Ignore_p -> HuML.Ignore_p
  in
  match e with
  | HuML.Type_e -> HuML.Type_e
  | HuML.TypeAnnot_e(e,t) -> HuML.TypeAnnot_e(rename x y e, rename x y t)
  | HuML.Op_e(e1, op, e2) ->  HuML.Op_e(rename x y e1, op, rename x y e2)
  | HuML.Var_e z -> if z = x then HuML.Var_e y else e
  | HuML.Int_e i-> HuML.Int_e i
  | HuML.Let_e(z,e1,e2) -> HuML.Let_e(z, rename x y e1,  if z = x then e2 else rename x y e2)
  | HuML.Lam_e(z,e) -> HuML.Lam_e(z, if z = x then e else rename x y e)
  | HuML.LamWithAnnot_e(z,t,e) -> HuML.LamWithAnnot_e(z, rename x y t, if z = x then e else rename x y e)
  | HuML.App_e(e1,e2) -> HuML.App_e(rename x y e1, rename x y e2)
  | HuML.If_e(c,e1,e2) -> HuML.If_e(rename x y c, rename x y e1, rename x y e2)
  | HuML.Match_e(e,es) -> HuML.Match_e(rename x y e, List.map (fun (p,e) ->
      (rename' p, rename x y e)) es) (* TODO: consider binding of patterns *)

(** checks whether e is a value *)
let is_value (e:HuML.exp) : bool =
  match e with
  |(HuML.Type_e
   | HuML.TypeAnnot_e _
   | HuML.Int_e _
   | HuML.Lam_e _
   | HuML.LamWithAnnot_e _) -> true
  | HuML.(Op_e _
   | HuML.Let_e _
   | HuML.App_e _
   | HuML.If_e _
   | HuML.Var_e _
   | HuML.Match_e _) -> false

(** substitutes v for x in e *)
let substitute (v:HuML.exp) (x:HuML.var) (e:HuML.exp) : HuML.exp =
  let rec subst (e:HuML.exp) : HuML.exp =
        match e with
        | HuML.Int_e _ -> e
        | HuML.Type_e -> e
        | HuML.TypeAnnot_e(e,t) -> HuML.TypeAnnot_e(subst e, subst t)
        | HuML.Op_e(e1,op,e2) -> HuML.Op_e(subst e1,op,subst e2)
        | HuML.Var_e y -> if x = y then v else e
        | HuML.Let_e(y,e1,e2) -> HuML.Let_e(y,
                                  subst e1,
                                  if x = y then e2 else subst e2)
        | HuML.App_e(e1,e2) -> HuML.App_e(subst e1,subst e2)
        | HuML.If_e(c,e1,e2) -> HuML.If_e(subst c, subst e1, subst e2)
        | HuML.Match_e(e,es) -> HuML.Match_e(subst e, List.map (fun (p,e) -> (p, subst e)) es) (* TODO: consider binding from patterns *)
        | HuML.Lam_e(y,e') ->
          if x = y then
            HuML.Lam_e(y, e')
          else if not (HuML.VarSet.mem y (fv v)) then
            HuML.Lam_e(y, subst e')
          else
            let fresh = HuML.gensym () in
            let new_body = rename y fresh e' in
            HuML.Lam_e (fresh, subst new_body)
        | HuML.LamWithAnnot_e(y,t,e') ->
          if x = y then
            HuML.LamWithAnnot_e(y, subst t, e')
          else if not (HuML.VarSet.mem y (fv v)) then
            HuML.LamWithAnnot_e(y, subst t, subst e')
          else
            let fresh = HuML.gensym () in
            let new_body = rename y fresh e' in
            HuML.LamWithAnnot_e (fresh, subst t, subst new_body)
  in
    subst e

(** prints a pattern *)
let rec print_pattern (p:HuML.pattern) : unit =
  match p with
  | HuML.Int_p i -> Printf.printf "%d" i
  | HuML.Ignore_p -> Printf.printf "_"
  | HuML.Var_p z -> Printf.printf "%s" z
  | HuML.App_p(p1,p2) -> print_pattern p1; Printf.printf " "; print_pattern p2

(** prints an expression *)
let rec print_exp (e:HuML.exp) : unit =
  match e with
  | HuML.Int_e i -> Printf.printf "%d" i
  | HuML.Type_e -> Printf.printf "Type"
  | HuML.TypeAnnot_e(e,t) ->
    Printf.printf "(";
    print_exp e;
    Printf.printf " : ";
    print_exp t;
    Printf.printf ")"
  | HuML.Op_e(e1,op,e2) ->
    Printf.printf "(";
    print_exp e1;
    Printf.printf " %s " (match op with
        | HuML.Plus -> "+"
        | HuML.Minus -> "-"
        | HuML.Mult -> "*"
        | HuML.Div -> "/"
        | HuML.Le -> "<"
        | HuML.LeEq -> "<="
        | HuML.GtEq -> ">="
        | HuML.Gt -> ">");
    print_exp e2;
    Printf.printf ")"
  | HuML.Let_e(x,e1,e2) ->
    Printf.printf "(let %s = " x;
    print_exp e1;
    Printf.printf "; ";
    print_exp e2;
    Printf.printf ")"
  | HuML.App_e(e1,e2) ->
    Printf.printf "(";
    print_exp e1;
    Printf.printf " ";
    print_exp e2;
    Printf.printf ")"
  | HuML.LamWithAnnot_e(x,t,e) ->
    Printf.printf "(\\%s : " x;
    print_exp t;
    Printf.printf ". ";
    print_exp e;
    Printf.printf ")"
  | HuML.Lam_e(x,e) ->
    Printf.printf "(\\%s. " x;
    print_exp e;
    Printf.printf ")"
  | HuML.Var_e(x) -> Printf.printf "%s" x
  | HuML.Match_e(e,es) ->
    Printf.printf "match ";
    print_exp e;
    Printf.printf " [";
    let len = List.length es in
    List.iteri (fun i -> fun (p,e) ->
        print_pattern p;
        Printf.printf " => ";
        print_exp e;
        if i + 1 < len then
          Printf.printf " | "
        else
          ()
      ) es;
    Printf.printf "] ";
  | HuML.If_e(c,e1,e2) ->
    Printf.printf "if ";
    print_exp c;
    Printf.printf " then ";
    print_exp e1;
    Printf.printf " else ";
    print_exp e2

(** prints a statement *)
let print_stmt (s:HuML.stmt) : unit =
  match s with
  | HuML.Expr_s e ->
    print_exp e;
    Printf.printf ";"
  | HuML.Assign_s(x,e) ->
    Printf.printf "%s = " x;
    print_exp e;
    Printf.printf ";"
  | HuML.Type_s(x,e) ->
    Printf.printf "type %s : " x;
    print_exp e;
    Printf.printf ";"
  | HuML.Data_s(x,e) ->
    Printf.printf "data %s : " x;
    print_exp e;
    Printf.printf ";"

(** Pretty prints the context of an evaluation context *)
let print_ctx (ctx:HuML.Evalcontext.t) : unit =
  let rec print_ctx' (ctx':HuML.Evalcontext.t) : unit =
    match ctx' with
    | [] -> ()
    | (z,x) :: xs ->
      Printf.printf "%s <- " z;
      print_exp x;
      Printf.printf ", ";
      print_ctx' xs
  in
    Printf.printf "[";
    print_ctx' ctx;
    Printf.printf "]";
