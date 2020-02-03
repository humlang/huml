#!/usr/bin/env bats

@test "block expects lbrace" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
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

