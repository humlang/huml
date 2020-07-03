#include <arguments_parser.hpp>
#include <diagnostic.hpp>
#include <compiler.hpp>
#include <config.hpp>

#include <backend/builder.hpp>
#include <backend/pe.hpp>
#include <backend/cogen.hpp>
#include <iostream>

int main(int argc, const char** argv)
{
  {
  ir::builder mach;

  auto Int = mach.i(false, mach.lit(32));
  auto Char = mach.i(false, mach.lit(8));

  auto ret = mach.param(mach.fn({mach.param(Int)}, mach.bot()));

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

  ir::cogen("b", entry);
  //  mach.print_graph(std::cout, foo);
  }
  /*
  {
  ir::builder mach;

  auto Int = mach.i(false, mach.lit(32));
  auto Char = mach.i(false, mach.lit(8));

  auto ret = mach.fn({mach.param(Int)}, mach.bot());

  /// fac(fac : int -> (int -> ⊥) -> ⊥, n : int, ret : int -> ⊥) return foox;
  auto n = mach.param(Int);
  auto fret = mach.fn({mach.param(Int)}, mach.bot());
  auto self = mach.fn({mach.param(Int), fret}, mach.bot());

  // n - 1
  auto pred = mach.binop(ir::BinaryKind::Minus, n, mach.lit(1));

  // case n [ 0 => fret 1 | _ => self (n - 1) (λm. fret (n * m)) ]
  auto fac = mach.fn({self, n, fret}, mach.destruct(n, {
        { mach.lit(0),   mach.app(ret, { mach.lit(1) }) },
        { mach.ignore(), mach.app(self, { pred }) }
        }));

  // main(argc : int, argv : char **) return bar(foo)
  auto argc = mach.param(Int);
  auto argv = mach.param(mach.ptr(mach.ptr(Char)));

  // fixpoint combinator for cbv languages
  auto Z = mach.id("_Z", nullptr);

  auto entry = mach.fn(mach.tup({ argc, argv }),
                       mach.app(fac, {mach.app(Z, fac), argc, ret }));
  entry->make_external("main");

  ir::cogen("a", entry);
  //  mach.print_graph(std::cout, foo);
  }
  */
}

/*
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
*/

