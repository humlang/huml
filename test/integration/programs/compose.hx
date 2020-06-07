

type Nat : Type;
data Zero : Nat;
data Succ : Nat -> Nat;

idnat = \(x : Nat). x;
inc = \(x : Nat). Succ x;


data Compose : \(A : Type). \(B : Type). \(C : Type).
               \(f : B -> C).
               \(g : A -> B).
               \(x : A).
               f (g x);

Compose Nat Nat Nat idnat inc Zero;

