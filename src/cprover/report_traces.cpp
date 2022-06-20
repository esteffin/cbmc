/*******************************************************************\

Module: Report Traces

Author:

\*******************************************************************/

/// \file
/// Solver

#include "report_traces.h"

#include "console.h"

#include <iomanip>

void show_trace(
  const std::vector<framet> &frames,
  const propertyt &property,
  const namespacet &ns)
{
  irep_idt function_id, file;

  // the path goes backwards, we want a forwards trace
  for(auto r_it = property.path.rbegin(); r_it != property.path.rend(); ++r_it)
  {
    const auto &frame = frames[r_it->index];

    if(
      frame.source_location.get_function() != function_id ||
      frame.source_location.get_file() != file)
    {
      consolet::out() << consolet::faint << frame.source_location.get_file();
      if(frame.source_location.get_function() != "")
        consolet::out() << " function " << frame.source_location.get_function();
      consolet::out() << consolet::reset << '\n';
      file = frame.source_location.get_file();
      function_id = frame.source_location.get_function();
    }

    consolet::out() << consolet::faint << std::setw(4)
                    << frame.source_location.get_line() << consolet::reset;
    consolet::out() << '\n';
  }
}

void report_traces(
  const std::vector<framet> &frames,
  const std::vector<propertyt> &properties,
  const namespacet &ns)
{
  for(auto &property : properties)
  {
    if(property.status == propertyt::REFUTED)
    {
      consolet::out() << '\n'
                      << "Trace for " << consolet::bold
                      << property.property_id() << consolet::reset << ':'
                      << '\n';

      show_trace(frames, property, ns);
    }
  }
}
