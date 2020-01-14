#pragma once

#include <functional>
#include <mutex>

#include <ast.hpp>

// The recursor allows us to recursively call ast_printer_helper
template<typename PrinterFn>
inline static constexpr auto ast_printer_helper = base_visitor {
  [](auto recursor, auto& arg) { PrinterFn print;
    print("unknown argument");
  },

  [](auto recursor, rec_wrap_t<literal>& lit) { PrinterFn print;
    print("literal token at " + lit->loc().to_string());
  },

  [](auto recursor, rec_wrap_t<identifier>& id) { PrinterFn print;
    print("identifier token at " + id->loc().to_string());
  },

  [](auto recursor, rec_wrap_t<loop>& l) { PrinterFn print;
    print("loop at " + l->loc().to_string());
  },

  [](auto recursor, rec_wrap_t<block>& b) { PrinterFn print;
    print("loop at " + b->loc().to_string());
  }
};

std::mutex ast_printer_mutex;

// The actual ast_printer now just needs to plug everything accordingly
template<typename PrinterFn>
inline static constexpr auto ast_printer = 
			[](auto& arg)
      {
        const std::lock_guard<std::mutex> lock(ast_printer_mutex);
        return ast_printer_helper<PrinterFn>(ast_printer_helper<PrinterFn>, arg);
      };
