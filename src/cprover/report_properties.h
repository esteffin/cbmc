/*******************************************************************\

Module: Property Reporting

Author:

\*******************************************************************/

/// \file
/// Property Reporting

#ifndef CPROVER_CPROVER_REPORT_PROPERTIES_H
#define CPROVER_CPROVER_REPORT_PROPERTIES_H

#include "solver.h"
#include "solver_types.h"

void report_properties(const std::vector<propertyt> &);

solver_resultt overall_outcome(const std::vector<propertyt> &);

#endif // CPROVER_CPROVER_REPORT_PROPERTIES_H
