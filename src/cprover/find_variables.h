/*******************************************************************\

Module: Find Variables

Author:

\*******************************************************************/

/// \file
/// Find Variables

#ifndef CPROVER_CPROVER_FIND_VARIABLES_H
#define CPROVER_CPROVER_FIND_VARIABLES_H

#include <util/std_expr.h>

#include <unordered_set>

std::unordered_set<symbol_exprt, irep_hash>
find_variables(const std::vector<exprt> &);

#endif // CPROVER_CPROVER_FIND_VARIABLES_H
