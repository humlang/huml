#pragma once

#include <functional>
#include <cassert>
#include <mutex>

#include <ast.hpp>
#include <ir.hpp>


/**
 *  Due to function calls, we need to perform the lowering in two basic steps.
 *   1) Just move any node into the ir.
 *   2) Make a second pass where we fix the additional_references, e.g. for function calls
 */

inline static constexpr auto ast_lowering_helper = base_visitor {
  // always add the following four edge cases
  [](auto&& rec, const std::monostate& t) -> hx_ir& { assert(false && "Should never be called.");
    return rec.state.ir;
  },
  [](auto&& rec, auto& arg) -> hx_ir& {
    assert(false && "Cannot lower this.");
    return rec.state.ir;
  },
  [](auto&& rec, const stmt_type& typ) -> hx_ir& {
    return std::visit(rec, typ);
  },
  [](auto&& rec, const exp_type& typ) -> hx_ir& {
    return std::visit(rec, typ);
  },

  [](auto&& rec, const literal& lit) -> hx_ir& { 
    rec.state.ir.kinds.emplace_back(IRNodeKind::literal);
    rec.state.ir.references.emplace_back(0);
    rec.state.ir.data.emplace_back(IRData{ lit->tok });

    return rec.state.ir;
  },

  [](auto&& rec, const identifier& id) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const unit& id) -> hx_ir& { 
    return rec.state.ir;
  },

  [](auto&& rec, const top& id) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const bot& id) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const error& err) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const expr_stmt& rd) -> hx_ir& {
    return rec.state.ir;
  },


  [](auto&& rec, const pattern& rd) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const match& pt) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const assign& ass) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const assign_type& ass) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const pattern_matcher& l) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const block& b) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const tuple& b) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const access& a) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const lambda& a) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const app& a) -> hx_ir& {
    return rec.state.ir;
  },

  [](auto&& rec, const binary_exp& bin) -> hx_ir& {
    std::visit(rec, rec.state.w[bin->get_left_exp()]);
    std::visit(rec, rec.state.w[bin->get_right_exp()]);

    rec.state.ir.kinds.emplace_back(IRNodeKind::binary_exp);
    rec.state.ir.references.emplace_back(2);
    rec.state.ir.data.emplace_back(IRData{ bin->tok });

    return rec.state.ir;
  }
};

std::mutex ast_lowering_mutex;

// The actual ast_printer now just needs to plug everything accordingly
inline static constexpr auto ast_lowering = 
			[](auto& v, const auto& arg) -> hx_ir
      {
        struct state
        {
          decltype(v)& w;
          hx_ir ir;
          std::vector<identifier> binder_stack;
        };

        auto lam = [&v](const auto& arg) -> hx_ir
        {
          const std::lock_guard<std::mutex> lock(ast_lowering_mutex);
          return ast_lowering_helper(stateful_recursor(state{v}, ast_lowering_helper), arg);
        };

        return std::visit(lam, arg);
      };

