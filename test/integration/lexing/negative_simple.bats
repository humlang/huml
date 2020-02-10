#!/usr/bin/env bats

@test "lex not a number" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=tokens << EOF
123abc
EOF

  echo "$output" | grep "PA-NN-000"
}

@test "do not lex octal numbers" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=tokens << EOF
0123
EOF

  echo "$output" | grep "PA-LZ-000"
}

