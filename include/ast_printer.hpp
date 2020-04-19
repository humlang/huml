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
inline static constexpr auto ast_printer_helper = base_visitor {
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
    print("< literal, id=\"" + std::to_string(lit->id()) + "\""
        + ", location=\"" + lit->loc().to_string() + "\""
        + ", symbol=\"" + lit->symb().get_string() + "\""
        + ">", rec.state.depth);
  },

  [](auto&& rec, const identifier& id) -> void { PrinterFn print;
    print("< identifier, id=\"" + std::to_string(id->id()) + "\""
        + ", location=\"" + id->loc().to_string() + "\""
        + ", symbol=\"" + id->symb().get_string() + "\""
        + ">", rec.state.depth);
  },

  [](auto&& rec, const unit& id) -> void { PrinterFn print;
    print("< unit, id=\"" + std::to_string(id->id()) + "\""
        + ", location=\"" + id->loc().to_string() + "\""
        + ">", rec.state.depth);
  },

  [](auto&& rec, const top& id) -> void { PrinterFn print;
    print("< TOP, id=\"" + std::to_string(id->id()) + "\""
        + ", location=\"" + id->loc().to_string() + "\""
        + ">", rec.state.depth);
  },

  [](auto&& rec, const bot& id) -> void { PrinterFn print;
    print("< BOT, id=\"" + std::to_string(id->id()) + "\""
        + ", location=\"" + id->loc().to_string() + "\""
        + ">", rec.state.depth);
  },

  [](auto&& rec, const error& err) -> void { PrinterFn print;
    print("< error, id=\"" + std::to_string(err->id()) + "\""
        + ", location=\"" + err->loc().to_string() + "\""
        + ", symbol=\"" + err->symb().get_string() + "\""
        + ">", rec.state.depth);
  },

  [](auto&& rec, const expr_stmt& rd) -> void { PrinterFn print;
    print("<|expr_stmt, id=\"" + std::to_string(rd->id()) + "\""
        + ", location=\"" + rd->loc().to_string() + "\""
        + ", symbol=\"" + rd->symb().get_string() + "\""
        + ", ", rec.state.depth);

    rec.state.depth++;
    std::visit(rec, rd->exp());
    rec.state.depth--;

    print("|expr_stmt>", rec.state.depth);
  },


  [](auto&& rec, const pattern& rd) -> void { PrinterFn print;
    print("<|readin, id=\"" + std::to_string(rd->id()) + "\""
        + ", location=\"" + rd->loc().to_string() + "\""
        + ", symbol=\"" + rd->symb().get_string() + "\""
        + ", ", rec.state.depth);

    rec.state.depth++;
    std::visit(rec, rd->pattern());
    rec.state.depth--;

    print("|readin>", rec.state.depth);
  },

  [](auto&& rec, const match& pt) -> void { PrinterFn print;
    print("<|print, id=\"" + std::to_string(pt->id()) + "\""
        + ", location=\"" + pt->loc().to_string() + "\""
        + ", symbol=\"" + pt->symb().get_string() + "\""
        + ", ", rec.state.depth);

    rec.state.depth++;
    std::visit(rec, pt->pattern());
    std::visit(rec, pt->expression());
    rec.state.depth--;

    print("|print>", rec.state.depth);
  },

  [](auto&& rec, const assign& ass) -> void { PrinterFn print;
    print("<|assign, id=\"" + std::to_string(ass->id()) + "\""
        + ", location=\"" + ass->loc().to_string() + "\""
        + ", symbol=\"" + ass->symb().get_string() + "\""
        + ", ", rec.state.depth);

    rec.state.depth++;
    std::visit(rec, ass->var());
    std::visit(rec, ass->exp());
    rec.state.depth--;

    print("|assign>", rec.state.depth);
  },

  [](auto&& rec, const pattern_matcher& l) -> void { PrinterFn print;
    print("<|loop, id=\"" + std::to_string(l->id()) + "\""
        + ", location=\"" + l->loc().to_string() + "\""
        + ", symbol=\"" + l->symb().get_string() + "\""
        + ", ", rec.state.depth);

    rec.state.depth++;
    std::visit(rec, l->to_be_matched());
    for(auto& v : l->match_patterns())
      std::visit(rec, v);
    rec.state.depth--;

    print("|loop>", rec.state.depth);
  },

  [](auto&& rec, const block& b) -> void { PrinterFn print;
    print("<|block, id=\"" + std::to_string(b->id()) + "\""
        + ", location=\"" + b->loc().to_string() + "\""
        + ", symbol=\"" + b->symb().get_string() + "\""
        + ", ", rec.state.depth);

    rec.state.depth++;
    for(auto& v : b->expressions())
      std::visit(rec, v);
    rec.state.depth--;

    print("|block>", rec.state.depth);
  },

  [](auto&& rec, const tuple& b) -> void { PrinterFn print;
    print("<|block, id=\"" + std::to_string(b->id()) + "\""
        + ", location=\"" + b->loc().to_string() + "\""
        + ", symbol=\"" + b->symb().get_string() + "\""
        + ", ", rec.state.depth);

    rec.state.depth++;
    for(auto& v : b->expressions())
      std::visit(rec, v);
    rec.state.depth--;

    print("|block>", rec.state.depth);
  },

  [](auto&& rec, const access& a) -> void { PrinterFn print;
    print("<|access id=\"" + std::to_string(a->id()) + "\""
        + ", location=\"" + a->loc().to_string() + "\"", rec.state.depth);

    rec.state.depth++;
    std::visit(rec, a->tuple());
    std::visit(rec, a->accessed_at());
    rec.state.depth--;

    print("|access>", rec.state.depth);
  },

  [](auto&& rec, const lambda& a) -> void { PrinterFn print;
    print("<|lambda id=\"" + std::to_string(a->id()) + "\""
        + ", location=\"" + a->loc().to_string() + "\"", rec.state.depth);

    rec.state.depth++;
    rec(a->argument());
    std::visit(rec, a->fbody());
    rec.state.depth--;

    print("|lambda>", rec.state.depth);
  },

  [](auto&& rec, const app& a) -> void { PrinterFn print;
    print("<|app id=\"" + std::to_string(a->id()) + "\""
        + ", location=\"" + a->loc().to_string() + "\"", rec.state.depth);

    rec.state.depth++;
    std::visit(rec, a->fun());
    std::visit(rec, a->argument());
    rec.state.depth--;

    print("|app>", rec.state.depth);
  },

  [](auto&& rec, const binary_exp& bin) -> void { PrinterFn print;
    print("<|binary expression, id=\"" + std::to_string(bin->id()) + "\""
        + ", location=\"" + bin->loc().to_string() + "\""
        + ", symbol=\"" + bin->symb().get_string() + "\""
        + ", ", rec.state.depth);

    rec.state.depth++;
    std::visit(rec, bin->get_left_exp());
    std::visit(rec, bin->get_right_exp());
    rec.state.depth--;

    print("|binary expression>", rec.state.depth);
  }
};

std::mutex ast_printer_mutex;

// The actual ast_printer now just needs to plug everything accordingly
template<typename PrinterFn>
inline static constexpr auto ast_printer = 
			[](const auto& arg) -> void
      {
        struct state
        { std::size_t depth; };

        const std::lock_guard<std::mutex> lock(ast_printer_mutex);
        return ast_printer_helper<PrinterFn>(stateful_recursor(state{0}, ast_printer_helper<PrinterFn>), arg);
      };

