#pragma once

#include <functional>
#include <mutex>

#include <ast.hpp>

// The recursor allows us to recursively call ast_printer_helper
template<typename PrinterFn>
inline static constexpr auto ast_printer_helper = base_visitor {
  [](auto&& rec, auto& arg) { PrinterFn print;
    print("unknown argument");
  },

  [](auto&& rec, const stmt_type& typ) {
    std::visit(rec, typ);
  },

  [](auto&& rec, const rec_wrap_t<literal>& lit) { PrinterFn print;
    print("literal token at " + lit->loc().to_string());
  },

  [](auto&& rec, const rec_wrap_t<identifier>& id) { PrinterFn print;
    print("identifier token at " + id->loc().to_string());
  },

  [](auto&& rec, const rec_wrap_t<assign>& ass) { PrinterFn print;
      print("Assign token at " + ass->loc().to_string());
      std::visit(rec, ass->exp());
  },

  [](auto&& rec, const rec_wrap_t<loop>& l) { PrinterFn print;
    auto loc = l->loc().to_string();
    print("loop at " + loc);
    print("[" + loc + "] data 1/2  -  ");
    rec(l->num_times());

    print("[" + loc + "] data 2/2  -  ");
    std::visit(rec, l->loop_body());
  },

  [](auto&& rec, const rec_wrap_t<block>& b) { PrinterFn print;
    print("block at " + b->loc().to_string());
  },

  [](auto&& rec, const rec_wrap_t<binary_exp>& bin) { PrinterFn print;
      print("binary expresion at " + bin->loc().to_string());
      auto loc = bin->loc().to_string();
      print("Left Expression: [" + loc + "] data 1/2");
      auto symbol = bin->symb();
      print(symbol.get_string());
      std::visit(rec, bin->get_left_exp());
      print("Right Expression: [" + loc + "] data 2/2");
      std::visit(rec, bin->get_right_exp());
  },
  [](auto&& rec, std::monostate& t) {}
};

std::mutex ast_printer_mutex;

// The actual ast_printer now just needs to plug everything accordingly
template<typename PrinterFn>
inline static constexpr auto ast_printer = 
			[](const auto& arg) -> void
      {
        const std::lock_guard<std::mutex> lock(ast_printer_mutex);

        ast_printer_helper<PrinterFn>(recursor(ast_printer_helper<PrinterFn>), arg);
      };

