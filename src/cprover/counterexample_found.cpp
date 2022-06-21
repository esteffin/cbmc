/*******************************************************************\

Module: Counterexample Found

Author:

\*******************************************************************/

/// \file
/// Counterexample Found

#include "counterexample_found.h"

#include <util/cout_message.h>
#include <util/format_expr.h>
#include <util/simplify_expr.h>

#include <solvers/sat/satcheck.h>

#include "axioms.h"
#include "bv_pointers_wide.h"
#include "state.h"

#include <iostream>

void show_assignment(const bv_pointers_widet &solver)
{
#if 0
  for(auto &entry : solver.get_cache())
  {
    const auto &expr = entry.first;
    if(expr.id() == ID_and || expr.id() == ID_or || expr.id() == ID_not)
      continue;
    auto value = solver.l_get(entry.second);
    std::cout << "|| " << format(expr) << " --> " << value << "\n";
  }
#endif

#if 1
  for(auto &entry : solver.get_map().get_mapping())
  {
    const auto &identifier = entry.first;
    auto symbol = symbol_exprt(identifier, entry.second.type);
    auto value = solver.get(symbol);
    std::cout << "|| " << format(symbol) << " --> " << format(value) << "\n";
  }
#endif

  for(auto &entry : solver.get_symbols())
  {
    const auto &identifier = entry.first;
    auto value = solver.l_get(entry.second);
    std::cout << "|| " << identifier << " --> " << value << "\n";
  }
}

propertyt::tracet counterexample(
  const std::vector<framet> &frames,
  const workt &work,
  const bv_pointers_widet &solver,
  const axiomst &axioms)
{
  propertyt::tracet trace;

  // work.path goes backwards, we want a forwards trace
  for(auto r_it = work.path.rbegin(); r_it != work.path.rend(); ++r_it)
  {
    const auto &frame = frames[r_it->index];
    propertyt::trace_statet state;
    state.frame = *r_it;

    for(auto &implication : frame.implications)
    {
      if(implication.rhs.arguments().size() != 1)
        continue;
      auto &argument = implication.rhs.arguments().front();
      if(argument.id() == ID_update_state)
      {
        const auto &update_state = to_update_state_expr(argument);
        auto address = update_state.address();
        auto value = update_state.new_value();
        state.updates.emplace_back(address, value);
      }
      else if(argument.id() == ID_enter_scope_state)
      {
        // do we have a value?
        const auto &enter_scope_state = to_enter_scope_state_expr(argument);
        auto address = enter_scope_state.address();
        auto evaluate_expr = evaluate_exprt(enter_scope_state.state(), address);
        auto translated = axioms.translate(evaluate_expr);
        auto value = solver.get(translated);
        if(value.is_not_nil() && value != evaluate_expr)
          state.updates.emplace_back(address, value);
      }
    }

    trace.push_back(std::move(state));
  }

  return trace;
}

optionalt<propertyt::tracet> counterexample_found(
  const std::vector<framet> &frames,
  const workt &work,
  const std::unordered_set<symbol_exprt, irep_hash> &address_taken,
  bool verbose,
  const namespacet &ns)
{
  auto &f = frames[work.frame.index];

  for(const auto &implication : f.implications)
  {
    if(implication.lhs.is_true())
    {
      cout_message_handlert message_handler;
      message_handler.set_verbosity(verbose ? 10 : 1);
      satcheckt satcheck(message_handler);
      bv_pointers_widet solver(ns, satcheck, message_handler);
      axiomst axioms(solver, address_taken, verbose, ns);

      // These are initial states, i.e., true ⇒ SInitial(ς).
      // Ask the solver whether the invariant is 'true'.
      axioms.set_to_false(work.invariant);
      axioms.emit();

      switch(solver())
      {
      case decision_proceduret::resultt::D_SATISFIABLE:
        if(verbose)
          show_assignment(solver);
        return counterexample(frames, work, solver, axioms);
      case decision_proceduret::resultt::D_UNSATISFIABLE:
        break;
      case decision_proceduret::resultt::D_ERROR:
        throw "error reported by solver";
      }
    }
  }

  return {};
}
