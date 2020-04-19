#!/usr/bin/env bats


@test "simple expr stmt unit" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
() ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|expr_stmt
unit
|expr_stmt
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "simple expr stmt bot" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
BOT ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|expr_stmt
BOT
|expr_stmt
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "simple expr stmt top" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
TOP ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|expr_stmt
TOP
|expr_stmt
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "simple expr stmt" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
5 ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|expr_stmt
literal
|expr_stmt
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "simple expr stmt in parenthesis" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
( 5 ) ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|expr_stmt
literal
|expr_stmt
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "simple expr stmt in nested parenthesis" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
(((( 5 )))) ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|expr_stmt
literal
|expr_stmt
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "nested app expr stmt 1" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
a b c d e f g ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|expr_stmt
|app
identifier
|app
identifier
|app
identifier
|app
identifier
|app
identifier
|app
identifier
identifier
|app
|app
|app
|app
|app
|app
|expr_stmt
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "nested app expr stmt 2" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
((a b) c) ((d e) (f g)) ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|expr_stmt
|app
|app
|app
identifier
identifier
|app
identifier
|app
|app
|app
identifier
identifier
|app
|app
identifier
identifier
|app
|app
|app
|expr_stmt
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "assign with binary expression" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
x = 5 + 5;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|assign
identifier
|binary expression
literal
literal
|binary expression
|assign
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    echo $i
    test "1" -eq "$i"
  done
}

@test "access tuple" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
(0,1,2,3,4).(1 + 2 - 3) ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|expr_stmt
|access
|tuple
literal
literal
literal
literal
literal
|tuple
|binary expression
|binary expression
literal
literal
|binary expression
literal
|binary expression
|access
|expr_stmt
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "simple block" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
{ 0 ; 1 ; 2 } ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|expr_stmt
|block
literal
literal
literal
|block
|expr_stmt
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "nested simple block" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
{ { 0 } ; { { { 1 } } }; { 2 ; { 3 ; { 4 } ; 5 } ; 6 } ; 7 } ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|expr_stmt
|block
|block
literal
|block
|block
|block
|block
literal
|block
|block
|block
|block
literal
|block
literal
|block
literal
|block
literal
|block
literal
|block
literal
|block
|expr_stmt
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "y combinator" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
Y = \f. f (\x. f x x) (\x. f x x) ;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|assign
identifier
|lambda
identifier
|app
identifier
|app
|lambda
identifier
|app
identifier
|app
identifier
identifier
|app
|app
|lambda
|lambda
identifier
|app
identifier
|app
identifier
identifier
|app
|app
|lambda
|app
|app
|lambda
|assign
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "assign with number" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
x = 5;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|assign
identifier
literal
|assign
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    test "1" -eq "$i"
  done
}

@test "precedence of * in binary expression" {
  # run populates variables "output" and "status", containing the output and statuscode
  run "$HX_LANG_PATH"/hx-lang --emit=ast <<EOF
x = 5 * x + 5;
EOF

  echo "$output" > /tmp/hx_lang_test_out1
  cat <<EOF > /tmp/hx_lang_test_out2
|assign
identifier
|binary expression symbol=+
|binary expression symbol=*
literal
identifier
|binary expression
literal
|binary expression
|assign
EOF

  # This finds the strings given above in the output of hx-lang
  out=$(awk 'NR==FNR{patterns[FNR]=$1} FNR<NR{if (index($0,patterns[FNR])>0) print "1"; else print "0"}' /tmp/hx_lang_test_out2 /tmp/hx_lang_test_out1)

  rm /tmp/hx_lang_test_out1 /tmp/hx_lang_test_out2

  # Each line must contain '1'
  for i in $(echo $out); do
    echo $i
    test "1" -eq "$i"
  done
}
