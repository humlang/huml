#pragma once

#include <ast.hpp>

// https://en.cppreference.com/w/cpp/utility/variant/visit (16.10.19)
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template<typename PrinterFn>
inline static constexpr auto ast_printer = overloaded {
  [](auto arg) { PrinterFn print;
    print("unknown argument");
  },

  [](literal lit) { PrinterFn print;
    print("literal token at " + lit.loc().to_string());
  },

  [](identifier id) { PrinterFn print;
    print("identifier token at " + id.loc().to_string());
  }
};

