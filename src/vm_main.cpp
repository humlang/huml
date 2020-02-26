#include <string_view>
#include <vector>

#include <diagnostic.hpp>
#include <repl.hpp>

int main(int argc, char** argv)
{
  virt::REPL repl;

  repl.run();

  diagnostic.print(stdout);
  return diagnostic.error_code();
}

