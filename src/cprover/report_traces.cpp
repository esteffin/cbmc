/*******************************************************************\

Module: Report Traces

Author:

\*******************************************************************/

/// \file
/// Solver

#include "report_traces.h"

#include <util/format_expr.h>

#include "console.h"
#include "state.h"

#include <iomanip>

void show_trace(
  const std::vector<framet> &frames,
  const propertyt &property,
  const namespacet &ns)
{
  irep_idt function_id, file;

  for(auto &step : property.trace)
  {
    const auto &frame = frames[step.frame.index];

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
                    << frame.source_location.get_line() << consolet::reset
                    << ' ';

    if(step.updates.empty())
    {
      bool first = true;
      for(auto &implication : frame.implications)
      {
        if(first)
          first = false;
        else
        {
          consolet::out() << std::setw(4) << ' ';
        }
        consolet::out() << "constraint: " << format(implication.as_expr())
                        << '\n';
      }
    }
    else
    {
      bool first = true;
      for(auto &update : step.updates)
      {
        if(first)
          first = false;
        else
        {
          consolet::out() << std::setw(4) << ' ';
        }

        consolet::out() << format(update.address);
        consolet::out() << " := " << format(update.value);
        consolet::out() << '\n';
      }
    }
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
