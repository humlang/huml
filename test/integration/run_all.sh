#!/usr/bin/env bash

export HX_LANG_PATH=$(cd "$PWD"/../../build ; echo $PWD)
export HX_LANG_TEST=$(cd "$PWD"/../../test ; echo $PWD)

## PARSING

success=0
for i in parsing/*.bats; do
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
