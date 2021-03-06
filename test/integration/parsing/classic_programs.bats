#!/usr/bin/env bats

# @test "fibonacci" {
#   # currently, we are in progress of integrating this.
#   # The compiler needs to become a little bit more useful to support this.
#   #  i.e. we need:
#   #   - a sensible ast-printer with different levels of detail, so that writing tests is not too painful
#   #   - special flags for emitting the token stream, the ast, etc.
#   skip

#   # run populates variables "output" and "status", containing the output and statuscode
#   run "$HX_LANG_PATH"/hx-lang --emit-ast -f "$HX_LANG_TEST"/integration/programs/fibonacci.hx

#   tmp1=$(echo $output | md5sum)
#   tmp2=$(cat "$HX_LANG_TEST"/integration/parsing/fibonacci.hx-ast | md5sum)

#   [ "$tmp1" = "$tmp2" ] || ( echo -ne "Hx-lang output:\n$output\n" && test -n "" )
# }

