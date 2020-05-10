#include <arguments_parser.hpp>
#include <diagnostic.hpp>
#include <compiler.hpp>
#include <config.hpp>

int main(int argc, const char** argv)
{
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

