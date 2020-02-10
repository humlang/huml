#!/usr/bin/env bats

@test "numbers" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=tokens <<EOF
1 2 3 4 5 6 7 8 9 0
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
LiteralNumber
LiteralNumber
LiteralNumber
LiteralNumber
LiteralNumber
LiteralNumber
LiteralNumber
LiteralNumber
LiteralNumber
LiteralNumber
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

