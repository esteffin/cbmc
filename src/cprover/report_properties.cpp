/*******************************************************************\

Module: Solver

Author:

\*******************************************************************/

/// \file
/// Solver

#include "report_properties.h"

#include <util/cout_message.h>

#include "console.h"

#include <iomanip>

void report_properties(const std::vector<propertyt> &properties)
{
  irep_idt current_file, current_function;

  for(auto &property : properties)
  {
    const auto &l = property.source_location;

    if(l.get_function() != current_function)
    {
      if(!current_function.empty())
        consolet::out() << '\n';
      current_function = l.get_function();
      if(!current_function.empty())
      {
        current_file = l.get_file();
        if(!current_file.empty())
          consolet::out() << current_file << ' ';
        if(!l.get_function().empty())
          consolet::out() << "function " << l.get_function();
        consolet::out() << '\n';
      }
    }

    auto property_id = property.property_id();
    consolet::out() << consolet::faint << '[';
    if(property_id.empty())
      consolet::out() << '?';
    else
      consolet::out() << property_id;
    consolet::out() << ']' << consolet::reset;

    if(l.get_file() != current_file)
      consolet::out() << ' ' << l.get_file();

    if(!l.get_line().empty())
      consolet::out() << " line " << l.get_line();

    auto comment = l.get_comment();
    if(!comment.empty())
      consolet::out() << ' ' << comment;

    consolet::out() << ": ";

    switch(property.status)
    {
    case propertyt::PASS:
      consolet::out() << consolet::green << "SUCCESS" << consolet::reset;
      break;

    case propertyt::REFUTED:
      consolet::out() << consolet::red << "REFUTED" << consolet::reset;
      break;

    case propertyt::ERROR:
      consolet::out() << consolet::red << consolet::bold << "ERROR"
                      << consolet::reset;
      break;

    case propertyt::DROPPED:
      consolet::out() << consolet::red << consolet::bold << "DROPPED"
                      << consolet::reset;
      break;

    case propertyt::UNKNOWN:
      consolet::out() << consolet::yellow << "UNKNOWN" << consolet::reset;
      break;
    }

    consolet::out() << '\n';
  }
}

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

solver_resultt overall_outcome(const std::vector<propertyt> &properties)
{
  auto result = solver_resultt::ALL_PASS;

  for(auto &property : properties)
    if(property.status == propertyt::ERROR)
      result = solver_resultt::ERROR;
    else if(property.status == propertyt::DROPPED)
      result = solver_resultt::ERROR;
    else if(property.status == propertyt::REFUTED)
      result = solver_resultt::SOME_FAIL;

  return result;
}
