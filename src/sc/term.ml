open Ast

module TagBag = Set.Make(Int)

type termination = Stop | Continue of Sc.history

let term_tag_stmt (x : HuML.stmt) : int =
  0 (* TODO: fix this *)
let term_tag_expr (x : HuML.exp) : int =
  0 (* TODO: fix this *)

let tagBag (x : Sc.state) : TagBag.t =
  TagBag.singleton (term_tag_stmt x)

let terminate (h : Sc.history) (s : state) : termination =
  if List.find_opt (fun a -> tagBag a = tagBag s) h <> Option.None then
    Stop
  else
    Continue (s :: h)

