/*******************************************************************\

Module: Axioms

Author:

\*******************************************************************/

/// \file
/// Axioms

#ifndef CPROVER_CPROVER_AXIOMS_H
#define CPROVER_CPROVER_AXIOMS_H

#include <util/std_expr.h>

#include <solvers/decision_procedure.h>

#include "state.h"

#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class axiomst
{
public:
  axiomst(
    decision_proceduret &__dest,
    const std::unordered_set<symbol_exprt, irep_hash> &__address_taken,
    const namespacet &__ns)
    : dest(__dest), address_taken(__address_taken), ns(__ns)
  {
  }

  void set_to_true(exprt);
  void set_to_false(exprt);
  void emit();

protected:
  decision_proceduret &dest;
  const std::unordered_set<symbol_exprt, irep_hash> &address_taken;
  const namespacet &ns;

  std::vector<exprt> constraints;

  exprt replace(exprt);
  typet replace(typet);
  std::unordered_map<exprt, symbol_exprt, irep_hash> replacement_map;
  std::map<irep_idt, std::size_t> counters;

  void node(const exprt &);

  std::set<address_of_exprt> address_of_exprs;

  std::set<state_ok_exprt> ok_exprs;
  void ok_fc();

  std::set<evaluate_exprt> evaluate_exprs;
  void evaluate_fc();

  std::set<state_is_cstring_exprt> is_cstring_exprs;
  void is_cstring_fc();

  std::set<state_is_dynamic_object_exprt> is_dynamic_object_exprs;
  void is_dynamic_object_fc();

  std::set<state_live_object_exprt> live_object_exprs;
  void live_object_fc();

  std::set<state_object_size_exprt> object_size_exprs;
  void object_size();
  void object_size_fc();
};

static inline axiomst &operator<<(axiomst &axioms, exprt src)
{
  axioms.set_to_true(std::move(src));
  return axioms;
}

#endif // CPROVER_CPROVER_AXIOMS_H
