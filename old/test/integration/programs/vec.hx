

type Nat : Type;

data Zero : Nat;
data Succ : Nat -> Nat;

type Vec : \(A : Type). \(n : Nat). Type;

data Nil : \(A : Type). Vec A Zero;
data Cons : \(A : Type). \(n : Nat).
            \(x : A). \(xs : Vec A n). 
            Vec A (Succ n);

Nil Nat;
Nil Nat : Vec Nat Zero;

Cons Nat Zero 
    Zero
    (Nil Nat);

Cons Nat Zero
    Zero
    (Nil Nat) : Vec Nat (Succ Zero);

