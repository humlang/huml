
\(A : Type). \x. x;

\(A : Type). \(x : A). x;

\(A : Type). \(f : (A -> A) -> A). f (\(x : A). x);

\(A : Type). \f. (f : A);

\(A : Type). \(f : A -> A). \x. f x;

id = (\A. \x. x) : \(A : Type). \(x : A). A;

