#!/usr/bin/env bats

@test "block expects lbrace" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast << EOF
for 5 ; :=
EOF

  echo "$output" | grep "PA-BR-000"
}

@test "block expects rbrace" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
for 5 { :=
EOF

  echo "$output" | grep "PA-BR-001"
}

@test "assign expects colon-equal" {
    # run populates variables "output" and "status", containing the output and statuscode
      run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
x = 2;
EOF

      echo "$output" | grep "PA-BR-002"
}

@test "statement expects semicolon at end" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
x := 5
EOF

  echo "$output" | grep "PA-UT-000"
}

@test "not a number" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
x := 5t;
EOF

  echo "$output" | grep "PA-NN-000"
}

@test "module not empty" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
EOF

  echo "$output" | grep "PA-EM-000"
}