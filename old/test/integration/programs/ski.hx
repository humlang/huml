K = \(A : Type). \(B : Type). \(x : A). \(y : B). x;
I = \(A : Type). \(x : A). x;
S = \(A : Type). \(B : Type). \(C : Type). \(x : A -> B -> C). \(a : A -> B). \(b : A). x b (a b);
