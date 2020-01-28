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
  [](auto&& rec, auto& arg) -> void { PrinterFn print;
    print("unknown argument");
  },

  [](auto&& rec, const stmt_type& typ) -> void {
    std::visit(rec, typ);
  },

  [](auto&& rec, const literal& lit) -> void { PrinterFn print;
    print("literal token at " + lit->loc().to_string());
  },

  [](auto&& rec, const identifier& id) -> void { PrinterFn print;
    print("identifier token at " + id->loc().to_string());
  },

  [](auto&& rec, const assign& ass) -> void { PrinterFn print;
    std::visit(rec, ass->var());
    print("assign token at " + ass->loc().to_string());
    std::visit(rec, ass->exp());
  },

  [](auto&& rec, const loop& l) -> void { PrinterFn print;
    auto loc = l->loc().to_string();
    print("loop at " + loc);
    print("[" + loc + "] data 1/2  -  ");
    std::visit(rec, l->num_times());

    print("[" + loc + "] data 2/2  -  ");
    std::visit(rec, l->loop_body());
  },

  [](auto&& rec, const block& b) -> void { PrinterFn print;
    print("block at " + b->loc().to_string());
  },

  [](auto&& rec, const binary_exp& bin) -> void { PrinterFn print;
    print("binary expresion at " + bin->loc().to_string());
    auto loc = bin->loc().to_string();
    print("left expression: [" + loc + "] data 1/2");
    auto symbol = bin->symb();
    print(symbol.get_string());
    std::visit(rec, bin->get_left_exp());
    print("right expression: [" + loc + "] data 2/2");
    std::visit(rec, bin->get_right_exp());
  },
  [](auto&& rec, const std::monostate& t) -> void { assert(false && "Should never be called."); }
};

std::mutex ast_printer_mutex;

// The actual ast_printer now just needs to plug everything accordingly
template<typename PrinterFn>
inline static constexpr auto ast_printer = 
			[](const auto& arg) -> void
      {
        const std::lock_guard<std::mutex> lock(ast_printer_mutex);

        return ast_printer_helper<PrinterFn>(recursor(ast_printer_helper<PrinterFn>), arg);
      };

