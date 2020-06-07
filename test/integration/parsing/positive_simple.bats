#!/usr/bin/env bats

@test "simple expr stmt unit" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast-print <<EOF
() ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
();
EOF

  diff /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2
}


@test "SKI-calculus" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast-print <<EOF
K = \x. \y. x;
I = \x. x;
S = \x. \a. \b. x b (a b);
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
K = \x. \y. x;
I = \x. x;
S = \x. \a. \b. ((((x) (b))) (((a) (b))));
EOF

  diff /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2
}


@test "simple arrow" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast-print <<EOF
A
   ->
        A;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
(A -> A);
EOF

  diff /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2
}


@test "arrow in annot" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast-print <<EOF
\(A : Type). \(f : (A -> A) -> A). f (\(x : A). x);
\(A : Type). \(f : A -> (A -> A)). f (\(x : A). x);
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
\((A) : (Type)). \((f) : (((A -> A) -> A))). ((f) (\((x) : (A)). x));
\((A) : (Type)). \((f) : ((A -> (A -> A)))). ((f) (\((x) : (A)). x));
EOF

  diff /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2
}

