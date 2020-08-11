#!/usr/bin/env bats

@test "funny" {
  run "$HUML_PATH"/huml --funny 
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HUML_PATH"/huml -funny 
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HUML_PATH"/huml --f-u-n-n-y 
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HUML_PATH"/huml --f-u-n-n-y=
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HUML_PATH"/huml --f-u-n-n-y=====
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HUML_PATH"/huml --f-u-YYYy@y===-=-=
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HUML_PATH"/huml --f\$e\)\(\\-=-what
  ( echo -ne "$output" | grep "Unknown command line argument" )

  run "$HUML_PATH"/huml ----\$
  ( echo -ne "$output" | grep "Unknown command line argument" )
}


