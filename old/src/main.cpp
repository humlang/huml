#include <arguments_parser.hpp>
#include <diagnostic.hpp>
#include <compiler.hpp>
#include <config.hpp>

#include <backend/builder.hpp>
#include <backend/sc.hpp>
#include <backend/cogen.hpp>
#include <iostream>

int main(int argc, const char** argv)
{
#if 0
  {
  ir::builder mach;

  auto Int = mach.i(false, mach.lit(32));
  auto Char = mach.i(false, mach.lit(8));

  auto ret = mach.entry_ret();

  // int main(int, char**) { int x = argc; return x + 4; }
  auto fretx = mach.param(Int);
  auto fret = mach.param(mach.fn({mach.param(Int)}, mach.bot()));

  auto x = mach.param(Int);
  auto y = mach.param(Int);
  auto fx = mach.fn({x,y,fret}, mach.destruct(x, { { mach.lit(1), mach.app(fret, { mach.lit(42) })   },
                                                   { mach.lit(2), mach.app(fret, { y }) } }));
  auto argc = mach.param(Int);
  auto argv = mach.param(mach.ptr(mach.ptr(Char)));

  auto entry = mach.fn({argc, argv, ret}, mach.app(fx, { argc, mach.lit(29), ret }));
  entry->make_external("main");

  ir::supercompile(mach, entry);
  ir::cogen("b", entry);
  }
  {
  ir::builder mach;

  auto Int = mach.i(false, mach.lit(32));
  auto Char = mach.i(false, mach.lit(8));

  auto ret = mach.entry_ret();

  /// fac(fac : int -> (int -> ⊥) -> ⊥, n : int, ret : int -> ⊥) return foox;
  auto n = mach.param(Int);
  auto fret = mach.param(mach.fn({mach.param(Int)}, mach.bot()));

  auto self = mach.param(mach.fn({mach.param(Int), mach.param(Int)}, mach.bot()));

  auto i = mach.param(Int);
  auto r = mach.param(Int);
  auto help = mach.fn({self, i, r}, mach.destruct(i, { { mach.lit(0), mach.app(fret, {r}) }, 
                                                       { mach.ignore(), mach.app(self, { mach.binop(ir::BinaryKind::Minus, i, mach.lit(1)),
                                                                                         mach.binop(ir::BinaryKind::Mult, i, r) }) }}));
  // fixpoint combinator for cbv languages
  auto Z = mach.rec();
  auto fac = mach.fn({n, fret}, mach.app(help, {mach.app(Z, {help}), n, mach.lit(1)}));

  // main(argc : int, argv : char **) return bar(foo)
  auto argc = mach.param(Int);
  auto argv = mach.param(mach.ptr(mach.ptr(Char)));
  auto entry = mach.fn({ argc, argv, ret }, mach.app(fac, {argc, ret}));
  entry->make_external("main");

  ir::supercompile(mach, entry);
  ir::cogen("a", entry);
  }
}
#endif

  arguments::parse(argc, argv, stdout);

  if(diagnostic.error_code() != 0)
    goto end;
  if(config.print_help)
    goto end;

  { // <- needed for goto
  compiler comp;
  comp.go();
  }

end:
  diagnostic.print(stdout);
  return diagnostic.error_code();
}

