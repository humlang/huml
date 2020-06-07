

type Nat : Type;

data Zero : Nat;
data Succ : Nat -> Nat;


\(x : Nat). x;
(\x. x) : Nat -> Nat;

\x. Succ x;
(\x. Succ x) : Nat -> Nat;

