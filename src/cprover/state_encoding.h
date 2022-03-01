/*******************************************************************\

Module: State Encoding

Author: Daniel Kroening, dkr@amazon.com

\*******************************************************************/

/// \file
/// State Encoding

#ifndef CPROVER_CPROVER_STATE_ENCODING_H
#define CPROVER_CPROVER_STATE_ENCODING_H

#include "solver.h"

#include <iosfwd>

class goto_modelt;

enum class state_encoding_formatt
{
  ASCII,
  SMT2
};

void state_encoding(
  const goto_modelt &,
  state_encoding_formatt,
  bool program_is_inlined,
  std::ostream &out);

solver_resultt state_encoding_solver(
  const goto_modelt &,
  bool program_is_inlined,
  std::size_t loop_limit);

void variable_encoding(
  const goto_modelt &,
  state_encoding_formatt,
  std::ostream &out);

#endif // CPROVER_CPROVER_STATE_ENCODING_H
