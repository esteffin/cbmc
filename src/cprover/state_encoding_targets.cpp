/*******************************************************************\

Module: State Encoding Targets

Author:

\*******************************************************************/

#include "state_encoding_targets.h"

#include <util/format_expr.h>

#include <ostream>

void ascii_encoding_targett::set_to_true(source_locationt, exprt expr)
{
  counter++;
  if(counter < 10)
    out << ' ';
  out << '(' << counter << ')' << ' ';
  out << format(expr) << '\n';
}
