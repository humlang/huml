
type Bool : Type;

data true  : Bool;
data false : Bool;


type Nat : Type;

data zero : Nat;
data succ : \(_ : Nat). Nat;


type List : \(_ : Type). Type;

data nil  : \(A : Type). List A;
data cons : \(A : Type). \(a : A). \(as : List A). List A;


type Unit : \(A : Type). \(_ : A). A;


type eq : \(A : Type). \(x : A). \(_ : A). Prop;

data Q : \(A : Type). \(x : A). eq A x x;

