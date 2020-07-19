#!/usr/bin/env bats

@test "lex not a number" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HUML_PATH"/huml --emit=tokens << EOF
123abc
EOF

  echo "$output" | grep -i "not a number"
}

@test "do not lex octal numbers" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HUML_PATH"/huml --emit=tokens << EOF
0123
EOF

  echo "$output" | grep -i "leading zeros"
}

