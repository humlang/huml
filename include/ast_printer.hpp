#pragma once

#include <functional>

#include <ast.hpp>

// https://en.cppreference.com/w/cpp/utility/variant/visit (16.10.19)
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// The recursor allows us to recursively call ast_printer_helper
template<typename PrinterFn>
inline static constexpr auto ast_printer_helper = overloaded {
  [](auto recursor, auto arg) { PrinterFn print;
    print("unknown argument");
  },

  [](auto recursor, literal lit) { PrinterFn print;
    print("literal token at " + lit.loc().to_string());
  },

  [](auto recursor, identifier id) { PrinterFn print;
    print("identifier token at " + id.loc().to_string());
  }
};

// The actual ast_printer now just needs to plug everything accordingly
template<typename PrinterFn>
inline static constexpr auto ast_printer = 
			[](auto arg) { return ast_printer_helper<PrinterFn>(ast_printer_helper<PrinterFn>, arg); };
