#!/usr/bin/env bats

@test "not closed unit" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
(
EOF

  echo "$output" | grep -i "not a prefix"
}

@test "not closed parenthesized expression" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
( 5 
EOF

  echo "$output" | grep -i "expects a comma"
}

@test "not closed tuple" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
( 5, 4
EOF

  echo "$output" | grep -i "closing parenthesis"
}

@test "missing comma in tuple" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
( 5 4 )
EOF

  echo "$output" | grep -i "expects a comma"
}

@test "too many closed tuples" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
( 5, 4 ))))))))))))
EOF

  echo "$output" | grep -i "not a prefix"
}

@test "module not empty" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
EOF

  echo "$output" | grep -i "must not be empty"
}
