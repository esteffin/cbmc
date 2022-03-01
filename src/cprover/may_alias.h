/*******************************************************************\

Module: Solver

Author:

\*******************************************************************/

/// \file
/// May Alias

#ifndef CPROVER_CPROVER_MAY_ALIAS_H
#define CPROVER_CPROVER_MAY_ALIAS_H

#include <util/std_expr.h>

#include <unordered_set>

class namespacet;

bool permitted_by_strict_aliasing(const typet &, const typet &);

bool is_object_field_element(const exprt &);

// check whether to addresses are the same
optionalt<exprt> may_alias(
  const exprt &,
  const exprt &,
  const std::unordered_set<symbol_exprt, irep_hash> &address_taken,
  const namespacet &);

#endif // CPROVER_CPROVER_MAY_ALIAS_H
