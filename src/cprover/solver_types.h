/*******************************************************************\

Module: Solver

Author:

\*******************************************************************/

/// \file
/// Solver

#ifndef CPROVER_CPROVER_SOLVER_TYPES_H
#define CPROVER_CPROVER_SOLVER_TYPES_H

#include <util/mathematical_expr.h>
#include <util/std_expr.h>

#include <unordered_map>

class frame_reft
{
public:
  frame_reft() : index(0)
  {
  }
  explicit frame_reft(std::size_t __index) : index(__index)
  {
  }
  std::size_t index;
  friend bool operator==(const frame_reft &a, const frame_reft &b)
  {
    return a.index == b.index;
  }
};

using frame_mapt = std::unordered_map<symbol_exprt, frame_reft, irep_hash>;

class framet
{
public:
  framet(
    symbol_exprt __symbol,
    source_locationt __source_location,
    frame_reft __ref)
    : symbol(std::move(__symbol)),
      source_location(std::move(__source_location)),
      ref(std::move(__ref))
  {
  }

  symbol_exprt symbol;

  // our current hypothesis invariant
  std::vector<exprt> invariants;

  // auxiliary facts
  std::vector<exprt> auxiliaries;

  // formulas where this frame is on the rhs of ⇒
  struct implicationt
  {
    implicationt(exprt __lhs, function_application_exprt __rhs)
      : lhs(std::move(__lhs)), rhs(std::move(__rhs))
    {
    }
    exprt lhs;
    function_application_exprt rhs;
  };

  std::vector<implicationt> implications;

  // tracking source code origin
  source_locationt source_location = source_locationt::nil();

  void add_invariant(exprt);
  void add_auxiliary(exprt);

  void reset()
  {
    invariants.clear();
    auxiliaries.clear();
  }

  frame_reft ref;
};

class propertyt
{
public:
  propertyt(
    source_locationt __source_location,
    frame_reft __frame,
    exprt __condition)
    : source_location(std::move(__source_location)),
      frame(std::move(__frame)),
      condition(std::move(__condition))
  {
  }

  irep_idt property_id() const
  {
    return source_location.get_property_id();
  }

  source_locationt source_location;
  frame_reft frame;
  exprt condition;

  using statust = enum { UNKNOWN, PASS, REFUTED, ERROR, DROPPED };
  statust status = UNKNOWN;

  // trace information when REFUTED
  using patht = std::vector<frame_reft>;
  patht path;
};

struct workt
{
  using patht = std::vector<frame_reft>;

  workt(frame_reft __frame, exprt __invariant, patht __path)
    : frame(std::move(__frame)),
      invariant(std::move(__invariant)),
      path(std::move(__path))
  {
  }

  // The frame where the invariant is to be established.
  frame_reft frame;
  exprt invariant;

  patht path;
};

#endif // CPROVER_CPROVER_SOLVER_TYPES_H
