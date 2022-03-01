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

bool counterexample_found(
  const std::vector<framet> &frames,
  const workt &work,
  const std::unordered_set<symbol_exprt, irep_hash> &address_taken,
  const namespacet &ns)
{
  auto &f = frames[work.frame.index];

  for(const auto &implication : f.implications)
  {
    if(implication.lhs.is_true())
    {
      cout_message_handlert message_handler;
      satcheckt satcheck(message_handler);
      bv_pointers_widet solver(ns, satcheck, message_handler);
      axiomst axioms(solver, address_taken, ns);

      // these are initial states, i.e., true ⇒ SInitial(ς)
      axioms.set_to_false(work.invariant);

      // ask the solver whether the invariant is 'true'
      axioms.emit();

      switch(solver())
      {
      case decision_proceduret::resultt::D_SATISFIABLE:
        show_assignment(solver);
        return true;
      case decision_proceduret::resultt::D_UNSATISFIABLE:
        break;
      case decision_proceduret::resultt::D_ERROR:
        throw "error reported by solver";
      }
    }
  }

  return false;
}
