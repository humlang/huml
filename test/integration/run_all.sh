#!/usr/bin/env bash

export HUML_PATH=$(cd "$PWD"/../../build ; echo $PWD)
export HUML_TEST=$(cd "$PWD"/../../test ; echo $PWD)

success=0
for i in cmdargs/*.bats lexing/*.bats parsing/*.bats; do
  echo "Start $i."
  bats $i

  if test 0 -eq "$?" && [ $success -eq 0 ]; then
    success=0
  else
    success=1
  fi

  echo "Stopped $i."
done

test $success -eq 0
