#pragma once

#include <functional>
#include <cassert>
#include <mutex>

#include <types.hpp>
#include <ast.hpp>
#include <per_statement_ir.hpp>


/**
 *  Due to function calls, we need to perform the lowering in two basic steps.
 *   1) Just move any node into the ir.
 *   2) Make a second pass where we fix the additional_references, e.g. for function calls
 */

inline static constexpr auto ast_lowering_helper = base_visitor {
  // always add the following four edge cases
  [](auto&& rec, const std::monostate& t) -> hx_per_statement_ir& { assert(false && "Should never be called.");
    return rec.state.ir;
  },
  [](auto&& rec, auto& arg) -> hx_per_statement_ir& {
    assert(false && "Cannot lower this.");
    return rec.state.ir;
  },
  [](auto&& rec, const stmt_type& typ) -> hx_per_statement_ir& {
    return std::visit(rec, typ);
  },
  [](auto&& rec, const exp_type& typ) -> hx_per_statement_ir& {
    return std::visit(rec, typ);
  },

  [](auto&& rec, const literal& lit) -> hx_per_statement_ir& { 
    rec.state.ir.kinds.emplace_back(IRNodeKind::literal);
    rec.state.ir.references.emplace_back(0);
    rec.state.ir.data.emplace_back(IRData{ lit->symb() });
    return rec.state.ir;
  },

  [](auto&& rec, const identifier& id) -> hx_per_statement_ir& {
    auto present = std::find_if(rec.state.binder_stack.rbegin(), rec.state.binder_stack.rend(),
        [&id](auto x) { return x.first == id->symb(); });

    rec.state.ir.kinds.push_back(IRNodeKind::identifier);
    rec.state.ir.references.push_back(0);
    if(present != rec.state.binder_stack.rend())
    {
      rec.state.ir.data.push_back(IRData{ id->symb(), static_cast<std::uint_fast32_t>(-1), { present->second } });
    }
    else
    {
      if(!rec.state.ir.free_variable_roots.contains(id->symb()))
      {
        // add free variable to our list
        rec.state.ir.free_variable_roots[id->symb()] = { rec.state.ir.kinds.size() - 1 };
      }
      rec.state.ir.data.push_back(IRData{ id->symb(), static_cast<std::uint_fast32_t>(-1),
          { static_cast<std::uint_fast32_t>(-rec.state.ir.free_variable_roots[id->symb()]) }});
    }
    return rec.state.ir;
  },

  [](auto&& rec, const unit& id) -> hx_per_statement_ir& { 
    rec.state.ir.kinds.push_back(IRNodeKind::unit);
    rec.state.ir.references.push_back(0);
    rec.state.ir.data.push_back(IRData{ "()" });
    return rec.state.ir;
  },

  [](auto&& rec, const top& id) -> hx_per_statement_ir& {
    rec.state.ir.kinds.push_back(IRNodeKind::top);
    rec.state.ir.references.push_back(0);
    rec.state.ir.data.push_back(IRData{ "TOP" });
    return rec.state.ir;
  },

  [](auto&& rec, const bot& id) -> hx_per_statement_ir& {
    rec.state.ir.kinds.push_back(IRNodeKind::bot);
    rec.state.ir.references.push_back(0);
    rec.state.ir.data.push_back(IRData{ "BOT" });
    return rec.state.ir;
  },

  [](auto&& rec, const error& err) -> hx_per_statement_ir& {
    assert(false && "Can't lower errornous asts.");
    return rec.state.ir;
  },

  [](auto&& rec, const expr_stmt& rd) -> hx_per_statement_ir& {
    std::visit(rec, rec.state.w[rd->exp()]);

    rec.state.ir.kinds.push_back(IRNodeKind::expr_stmt);
    rec.state.ir.references.push_back(1);
    rec.state.ir.data.emplace_back(IRData{ rd->symb() });

    rec.state.ir.node_name = nullptr;

    return rec.state.ir;
  },


  [](auto&& rec, const pattern& rd) -> hx_per_statement_ir& {
    std::visit(rec, rec.state.w[rd->pattern()]);

    rec.state.ir.kinds.push_back(IRNodeKind::pattern);
    rec.state.ir.references.push_back(1);
    rec.state.ir.data.emplace_back(IRData{ rd->symb() });
    return rec.state.ir;
  },

  [](auto&& rec, const match& pt) -> hx_per_statement_ir& {
    std::visit(rec, rec.state.w[pt->expression()]);
    std::visit(rec, rec.state.w[pt->pattern()]);

    rec.state.ir.kinds.push_back(IRNodeKind::match);
    rec.state.ir.references.push_back(2);
    rec.state.ir.data.emplace_back(IRData{ pt->symb() });
    return rec.state.ir;
  },

  [](auto&& rec, const assign& ass) -> hx_per_statement_ir& {
    std::visit(rec, rec.state.w[ass->exp()]);

    rec.state.ir.node_name = std::make_shared<symbol>(std::get<identifier>(rec.state.w[ass->var()])->symb());

    rec.state.ir.kinds.push_back(IRNodeKind::assign);
    rec.state.ir.references.push_back(1);
    rec.state.ir.data.emplace_back(IRData{ ass->symb() });

    return rec.state.ir;
  },

  [](auto&& rec, const assign_type& ass) -> hx_per_statement_ir& {
    auto name_id_variant = rec.state.w[ass->name()];
    identifier name_id = std::get<identifier>(name_id_variant);

    std::vector<std::shared_ptr<constructors::base>> constructors;
    for(auto it = ass->constructors().begin(); it != ass->constructors().end(); ++it)
    {
      auto& x = *it;
      auto variant = rec.state.w[x];
      if(std::holds_alternative<identifier>(variant))
      {
        constructors.emplace_back(std::make_shared<constructors::identifier>(
                                    std::get<identifier>(variant)->symb()));
        std::visit(rec, variant);
      }
      else if(std::holds_alternative<app>(variant))
      {
        app appl = std::get<app>(variant);
        
        auto caller_variant = rec.state.w[appl->fun()];
        assert(std::holds_alternative<identifier>(caller_variant) && "Constructors need a name.");
        std::visit(rec, caller_variant);

        auto param_variant = rec.state.w[appl->argument()];

        // TODO: support more than just one argument constructors
        std::shared_ptr<constructors::base> param;
          if(std::holds_alternative<identifier>(param_variant))
          {
            param = std::make_shared<constructors::identifier>(std::get<identifier>(param_variant)->symb());
          }
          else if(std::holds_alternative<unit>(param_variant))
          {
            param = std::make_shared<constructors::unit>();
          }
          else if(std::holds_alternative<app>(param_variant))
          {
            assert(false && "unsupported");
          }
          else if(std::holds_alternative<tuple>(param_variant))
          {
            assert(false && "unsupported");
          }
        constructors.emplace_back(
            std::make_shared<constructors::app>(
                std::make_shared<constructors::identifier>(std::get<identifier>(caller_variant)->symb()),
                param
              ));
      }
      else
        assert(false && "Bug in parser. Constructors can only be identifiers or apps.");
    }
    rec.state.ir.kinds.emplace_back(IRNodeKind::assign_type);
    rec.state.ir.references.emplace_back(constructors.size());

    rec.state.ir.types.emplace_back(std::make_shared<inductive>(
          name_id->symb(),
          std::move(constructors)
        ));
    rec.state.ir.data.emplace_back(IRData{ name_id->symb(), rec.state.ir.types.size() - 1 });
    rec.state.ir.node_name = std::make_shared<symbol>(name_id->symb());
    return rec.state.ir;
  },

  [](auto&& rec, const pattern_matcher& l) -> hx_per_statement_ir& {
    auto& v = l->match_patterns();
    for(auto it = v.rbegin(); it != v.rend(); ++it)
      std::visit(rec, rec.state.w[*it]);

    std::visit(rec, rec.state.w[l->to_be_matched()]);
    
    rec.state.ir.kinds.emplace_back(IRNodeKind::pattern_matcher);
    rec.state.ir.references.emplace_back(v.size() + 1);
    rec.state.ir.data.emplace_back(IRData{ l->symb() }); // to be inferred
    return rec.state.ir;
  },

  [](auto&& rec, const block& b) -> hx_per_statement_ir& {
    auto& v = b->expressions();
    for(auto it = v.rbegin(); it != v.rend(); ++it)
      std::visit(rec, rec.state.w[*it]);
    
    rec.state.ir.kinds.emplace_back(IRNodeKind::block);
    rec.state.ir.references.emplace_back(v.size());
    rec.state.ir.data.emplace_back(IRData{ b->symb() }); // to be inferred
    return rec.state.ir;
  },

  [](auto&& rec, const tuple& b) -> hx_per_statement_ir& {
    auto& v = b->expressions();
    for(auto it = v.rbegin(); it != v.rend(); ++it)
      std::visit(rec, rec.state.w[*it]);
    
    rec.state.ir.kinds.emplace_back(IRNodeKind::tuple);
    rec.state.ir.references.emplace_back(v.size());
    rec.state.ir.data.emplace_back(IRData{ b->symb() }); // to be inferred
    return rec.state.ir;
  },

  [](auto&& rec, const access& a) -> hx_per_statement_ir& {
    std::visit(rec, rec.state.w[a->accessed_at()]);
    std::visit(rec, rec.state.w[a->tuple()]);

    rec.state.ir.kinds.emplace_back(IRNodeKind::access);
    rec.state.ir.references.emplace_back(2);
    rec.state.ir.data.emplace_back(IRData{ a->symb() }); // to be inferred
    return rec.state.ir;
  },

  [](auto&& rec, const lambda& a) -> hx_per_statement_ir& {
    auto psymb = std::get<identifier>(rec.state.w[a->argument()])->symb();
    rec.state.binder_stack.emplace_back(psymb, rec.state.ir.kinds.size() + 1);

    std::visit(rec, rec.state.w[a->fbody()]);

    rec.state.binder_stack.pop_back();

    rec.state.ir.kinds.emplace_back(IRNodeKind::param);
    rec.state.ir.references.emplace_back(0);
    rec.state.ir.data.emplace_back(IRData{ psymb });

    rec.state.ir.kinds.emplace_back(IRNodeKind::lambda);
    rec.state.ir.references.emplace_back(2);
    rec.state.ir.data.emplace_back(IRData{ a->symb() }); // to be inferred
    return rec.state.ir;
  },

  [](auto&& rec, const app& a) -> hx_per_statement_ir& {
    std::visit(rec, rec.state.w[a->argument()]);
    std::visit(rec, rec.state.w[a->fun()]);

    rec.state.ir.kinds.emplace_back(IRNodeKind::app);
    rec.state.ir.references.emplace_back(0);
    rec.state.ir.data.emplace_back(IRData{ a->symb() }); // to be inferred
    return rec.state.ir;
  },

  [](auto&& rec, const binary_exp& bin) -> hx_per_statement_ir& {
    std::visit(rec, rec.state.w[bin->get_right_exp()]);
    std::visit(rec, rec.state.w[bin->get_left_exp()]);

    rec.state.ir.kinds.emplace_back(IRNodeKind::binary_exp);
    rec.state.ir.references.emplace_back(2);
    rec.state.ir.data.emplace_back(IRData{ bin->symb() }); // to be inferred
    return rec.state.ir;
  }
};

std::mutex ast_lowering_mutex;

// The actual ast_printer now just needs to plug everything accordingly
inline static constexpr auto ast_lowering = 
			[](auto& v, const auto& arg) -> hx_per_statement_ir
      {
        struct state
        {
          decltype(v)& w;
          hx_per_statement_ir ir;
          std::vector<std::pair<symbol, std::uint_fast32_t>> binder_stack;
        };

        auto lam = [&v](const auto& arg) -> hx_per_statement_ir
        {
          const std::lock_guard<std::mutex> lock(ast_lowering_mutex);
          return ast_lowering_helper(stateful_recursor(state{v}, ast_lowering_helper), arg);
        };

        return std::visit(lam, arg);
      };

