open Ast
open Term

type state = HuML.stmt
type history = state list


module ScStore = struct
  let history_hash = Hashtbl.create 123456
end


let memo (f : state -> HuML.stmt) (s : state) : HuML.stmt =
  let res = f s
  in
    Hashtbl.add ScStore.history_hash s res;
    res

let rec sc (h : history) (s : state) : HuML.stmt =
  let rec sc' (h' : history) (s' : state) : HuML.stmt =
    match Term.terminate h' s' with
    | Continue h'' -> split (sc h'') (reduce state)
    | Stop -> split (sc h') state
  in
    memo (sc' hist)

let start (p:HuML.program) : HuML.program =
  (* TODO: Do we benefit if we keep the history, i.e. sc returns it and we refeed it into it? *)
  List.fold_left (fun b -> fun a -> b @ [sc [] a]) [] p
