/*******************************************************************\

Module: Solver

Author:

\*******************************************************************/

/// \file
/// Equality Propagation

#ifndef CPROVER_CPROVER_SOLVER_H
#define CPROVER_CPROVER_SOLVER_H

#include <util/expr.h>

enum class solver_resultt
{
  ALL_PASS,
  SOME_FAIL,
  ERROR
};

solver_resultt solver(
  const std::vector<exprt> &,
  const std::size_t loop_limit,
  const namespacet &);

#endif // CPROVER_CPROVER_SOLVER_H
