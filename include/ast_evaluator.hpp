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
inline static constexpr auto ast_evaluator_helper = base_visitor {
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
  },

  [](auto&& rec, const identifier& id) -> void { PrinterFn print;
  },

  [](auto&& rec, const error& err) -> void { PrinterFn print;
  },

  [](auto&& rec, const readin& rd) -> void { PrinterFn print;
  },

  [](auto&& rec, const print& pt) -> void { PrinterFn print;
  },

  [](auto&& rec, const assign& ass) -> void { PrinterFn print;
  },

  [](auto&& rec, const loop& l) -> void { PrinterFn print;
  },

  [](auto&& rec, const block& b) -> void { PrinterFn print;
  },

  [](auto&& rec, const binary_exp& bin) -> void { PrinterFn print;
  }
};

std::mutex ast_evaluator_mutex;

// The actual ast_printer now just needs to plug everything accordingly
template<typename PrinterFn>
inline static constexpr auto ast_evaluator = 
			[](const auto& arg) -> void
      {
        struct state
        {
          using Scope = std::unordered_map<std::string, std::size_t>;

          std::vector<Scope> scopes;
        };

        const std::lock_guard<std::mutex> lock(ast_evaluator_mutex);
        return ast_evaluator_helper<PrinterFn>(stateful_recursor(state{0}, ast_evaluator_helper<PrinterFn>), arg);
      };

