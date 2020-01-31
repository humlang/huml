#!/usr/bin/env bats

@test "funny" {
  run "$HX_LANG_PATH"/hx-lang --funny 
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HX_LANG_PATH"/hx-lang -funny 
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HX_LANG_PATH"/hx-lang --f-u-n-n-y 
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HX_LANG_PATH"/hx-lang --f-u-n-n-y=
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HX_LANG_PATH"/hx-lang --f-u-n-n-y=====
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HX_LANG_PATH"/hx-lang --f-u-YYYy@y===-=-=
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HX_LANG_PATH"/hx-lang --f\$e\)\(\\-=-what
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HX_LANG_PATH"/hx-lang ----\$
  ( echo -ne "$output" | grep "Unknown command line argument" )
}


