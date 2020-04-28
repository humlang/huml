#pragma once

#include <functional>
#include <cassert>
#include <mutex>

#include <ast.hpp>

// The recursor allows us to recursively call ast_printer_helper
// If you need to return stuff, all lambdas must return the same type
//  -> You can return multiple types by returning std::variant<T1, T2>
//   
//   Also note that we need to state the return type explicitly, otherwise you'll have nasty errors
template<typename PrinterFn>
inline static constexpr auto ast_pretty_printer_helper = base_visitor {
  // always add the following four edge cases
  [](auto&& rec, const std::monostate& t) -> void { assert(false && "Should never be called."); },
  [](auto&& rec, auto& arg) -> void { PrinterFn print;
    print("unknown argument", rec.state.depth);
  },
  [](auto&& rec, const stmt_type& typ) -> void {
    std::visit(rec, typ);
  },
  [](auto&& rec, const exp_type& typ) -> void {
    std::visit(rec, typ);
  },

  [](auto&& rec, const literal& lit) -> void { PrinterFn print;
    print(lit->symb().get_string());
  },

  [](auto&& rec, const identifier& id) -> void { PrinterFn print;
    print(id->symb().get_string());
  },

  [](auto&& rec, const unit& id) -> void { PrinterFn print;
    print("()");
  },

  [](auto&& rec, const top& id) -> void { PrinterFn print;
    print("TOP");
  },

  [](auto&& rec, const bot& id) -> void { PrinterFn print;
    print("BOT");
  },

  [](auto&& rec, const error& err) -> void { PrinterFn print;
    print("\n1_error\n");
  },

  [](auto&& rec, const expr_stmt& rd) -> void { PrinterFn print;
    rec.state.depth++;
    std::visit(rec, rd->exp());
    rec.state.depth--;

    print(";\n");
  },


  [](auto&& rec, const pattern& rd) -> void { PrinterFn print;
    rec.state.depth++;
    std::visit(rec, rd->pattern());
    rec.state.depth--;
  },

  [](auto&& rec, const match& pt) -> void { PrinterFn print;
    rec.state.depth++;
    std::visit(rec, pt->pattern());
    print(" => ");
    std::visit(rec, pt->expression());
    rec.state.depth--;
  },

  [](auto&& rec, const assign& ass) -> void { PrinterFn print;
    rec.state.depth++;
    std::visit(rec, ass->var());
    print(" = ");
    std::visit(rec, ass->exp());
    rec.state.depth--;

    print(";\n");
  },

  [](auto&& rec, const assign_type& ass) -> void { PrinterFn print;
    rec.state.depth++;
    std::visit(rec, ass->name());
    print(" = ");
    for(auto vit = ass->constructors().begin(); vit != ass->constructors().end(); ++vit)
    {
      auto& v = *vit;
      std::visit(rec, v);

      if(std::next(vit) != ass->constructors().end())
        print("\n| ");
    }
    rec.state.depth--;

    print(";\n");
  },

  [](auto&& rec, const pattern_matcher& l) -> void { PrinterFn print;
    rec.state.depth++;
    print("case ");
    std::visit(rec, l->to_be_matched());
    print(" [\n");
    rec.state.depth++;

    bool printed_newline = false;
    for(auto vit = l->match_patterns().begin(); vit != l->match_patterns().end(); ++vit)
    {
      auto& v = *vit;
      std::visit(rec, v);

      if(std::next(vit) != l->match_patterns().end())
      {
        print("\n| ");
        printed_newline = true;
      }
    }
    rec.state.depth--;
    rec.state.depth--;

    if(printed_newline)
      print("\n]");
    else
      print("]");
  },

  [](auto&& rec, const block& b) -> void { PrinterFn print;
    rec.state.depth++;

    print("{ ");
    for(auto vit = b->expressions().begin(); vit != b->expressions().end(); ++vit)
    {
      auto& v = *vit;
      std::visit(rec, v);

      if(std::next(vit) != b->expressions().end())
        print(" ; ");
    }
    print(" } ");
    rec.state.depth--;
  },

  [](auto&& rec, const tuple& b) -> void { PrinterFn print;
    rec.state.depth++;

    print("(");
    for(auto vit = b->expressions().begin(); vit != b->expressions().end(); ++vit)
    {
      auto& v = *vit;
      std::visit(rec, v);

      if(std::next(vit) != b->expressions().end())
        print(", ");
    }
    print(")");

    rec.state.depth--;
  },

  [](auto&& rec, const access& a) -> void { PrinterFn print;
    rec.state.depth++;

    print("(");
    std::visit(rec, a->tuple());
    print(").(");
    std::visit(rec, a->accessed_at());
    print(")");

    rec.state.depth--;
  },

  [](auto&& rec, const lambda& a) -> void { PrinterFn print;
    rec.state.depth++;
    print("(\\");
    rec(a->argument());
    print(". ");
    std::visit(rec, a->fbody());
    print(")");
    rec.state.depth--;
  },

  [](auto&& rec, const app& a) -> void { PrinterFn print;
    rec.state.depth++;
    print("((");
    std::visit(rec, a->fun());
    print(") (");
    std::visit(rec, a->argument());
    print("))");
    rec.state.depth--;
  },

  [](auto&& rec, const binary_exp& bin) -> void { PrinterFn print;
    rec.state.depth++;

    print("(");
    std::visit(rec, bin->get_left_exp());
    print(") " + bin->symb().get_string() + " (");
    std::visit(rec, bin->get_right_exp());
    print(")");

    rec.state.depth--;
  }
};

std::mutex ast_pretty_printer_mutex;

// The actual ast_printer now just needs to plug everything accordingly
template<typename PrinterFn>
inline static constexpr auto ast_pretty_printer = 
			[](const auto& arg) -> void
      {
        struct state
        { std::size_t depth; };

        const std::lock_guard<std::mutex> lock(ast_pretty_printer_mutex);
        return ast_pretty_printer_helper<PrinterFn>(stateful_recursor(state{0}, ast_pretty_printer_helper<PrinterFn>), arg);
      };

