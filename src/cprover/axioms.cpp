/*******************************************************************\

Module: Axioms

Author:

\*******************************************************************/

/// \file
/// Axioms

#include "axioms.h"

#include <util/arith_tools.h>
#include <util/bitvector_expr.h>
#include <util/c_types.h>
#include <util/format_expr.h>
#include <util/pointer_predicates.h>
#include <util/simplify_expr.h>

#include "simplify_state_expr.h"
#include "state.h"

#include <iostream>

void axiomst::set_to_true(exprt src)
{
  constraints.push_back(std::move(src));
}

void axiomst::set_to_false(exprt src)
{
  set_to_true(not_exprt(src));
}

typet axiomst::replace(typet src)
{
  if(src.id() == ID_array)
  {
    auto &array_type = to_array_type(src);
    array_type.element_type() = replace(array_type.element_type());
    array_type.size() = replace(array_type.size());
    return src;
  }
  else if(src.id() == ID_pointer)
  {
    to_pointer_type(src).base_type() =
      replace(to_pointer_type(src).base_type());
    return src;
  }
  else
    return src;
}

void axiomst::evaluate_fc()
{
  // quadratic
  for(auto a_it = evaluate_exprs.begin(); a_it != evaluate_exprs.end(); a_it++)
  {
    for(auto b_it = std::next(a_it); b_it != evaluate_exprs.end(); b_it++)
    {
      if(a_it->state() != b_it->state())
        continue;

      auto a_op = a_it->address();
      auto b_op = typecast_exprt::conditional_cast(
        b_it->address(), a_it->address().type());
      auto operands_equal = equal_exprt(a_op, b_op);
      auto implication = implies_exprt(
        operands_equal,
        equal_exprt(
          *a_it, typecast_exprt::conditional_cast(*b_it, a_it->type())));
      std::cout << "EVALUATE: " << format(implication) << '\n';
      dest << replace(implication);
    }
  }
}

void axiomst::is_cstring_fc()
{
  // quadratic
  for(auto a_it = is_cstring_exprs.begin(); a_it != is_cstring_exprs.end();
      a_it++)
  {
    for(auto b_it = std::next(a_it); b_it != is_cstring_exprs.end(); b_it++)
    {
      if(a_it->state() != b_it->state())
        continue;
      auto a_op = a_it->address();
      auto b_op = typecast_exprt::conditional_cast(
        b_it->address(), a_it->address().type());
      auto operands_equal = equal_exprt(a_op, b_op);
      auto implication =
        implies_exprt(operands_equal, equal_exprt(*a_it, *b_it));
      std::cout << "IS_CSTRING: " << format(implication) << '\n';
      dest << replace(implication);
    }
  }
}

void axiomst::live_object_fc()
{
  // quadratic
  for(auto a_it = live_object_exprs.begin(); a_it != live_object_exprs.end();
      a_it++)
  {
    for(auto b_it = std::next(a_it); b_it != live_object_exprs.end(); b_it++)
    {
      if(a_it->state() != b_it->state())
        continue;
      auto operands_equal = same_object(a_it->address(), b_it->address());
      auto implication =
        implies_exprt(operands_equal, equal_exprt(*a_it, *b_it));
      std::cout << "LIVE_OBJECT: " << format(implication) << '\n';
      dest << replace(implication);
    }
  }
}

void axiomst::is_dynamic_object_fc()
{
  // quadratic
  for(auto a_it = is_dynamic_object_exprs.begin();
      a_it != is_dynamic_object_exprs.end();
      a_it++)
  {
    for(auto b_it = std::next(a_it); b_it != is_dynamic_object_exprs.end();
        b_it++)
    {
      if(a_it->state() != b_it->state())
        continue;
      auto operands_equal = same_object(a_it->address(), b_it->address());
      auto implication =
        implies_exprt(operands_equal, equal_exprt(*a_it, *b_it));
      std::cout << "IS_DYNAMIC_OBJECT: " << format(implication) << '\n';
      dest << replace(implication);
    }
  }
}

void axiomst::object_size()
{
  for(const auto &src : object_size_exprs)
  {
    auto src_simplified = simplify_state_expr_node(src, address_taken, ns);
    if(src_simplified != src)
    {
      auto equal = equal_exprt(src, src_simplified);
      std::cout << "OBJECT_SIZE: " << format(equal) << '\n';
      dest << replace(equal);
    }
  }
}

void axiomst::object_size_fc()
{
  // quadratic
  for(auto a_it = object_size_exprs.begin(); a_it != object_size_exprs.end();
      a_it++)
  {
    for(auto b_it = std::next(a_it); b_it != object_size_exprs.end(); b_it++)
    {
      if(a_it->state() != b_it->state())
        continue;
      auto operands_equal = same_object(a_it->address(), b_it->address());
      auto implication =
        implies_exprt(operands_equal, equal_exprt(*a_it, *b_it));
      std::cout << "OBJECT_SIZE: " << format(implication) << '\n';
      dest << replace(implication);
    }
  }
}

void axiomst::ok_fc()
{
  // quadratic
  for(auto a_it = ok_exprs.begin(); a_it != ok_exprs.end(); a_it++)
  {
    for(auto b_it = std::next(a_it); b_it != ok_exprs.end(); b_it++)
    {
      if(a_it->id() != b_it->id())
        continue;
      if(a_it->state() != b_it->state())
        continue;
      if(a_it->size() != b_it->size())
        continue;
      auto a_op = a_it->address();
      auto b_op = typecast_exprt::conditional_cast(
        b_it->address(), a_it->address().type());
      auto operands_equal = equal_exprt(a_op, b_op);
      auto implication =
        implies_exprt(operands_equal, equal_exprt(*a_it, *b_it));
      std::cout << "OK: " << format(implication) << '\n';
      dest << replace(implication);
    }
  }
}

exprt axiomst::replace(exprt src)
{
  src.type() = replace(src.type());

  if(
    src.id() == ID_evaluate || src.id() == ID_state_is_cstring ||
    src.id() == ID_state_is_dynamic_object ||
    src.id() == ID_state_object_size || src.id() == ID_state_live_object ||
    src.id() == ID_state_r_ok || src.id() == ID_state_w_ok ||
    src.id() == ID_state_rw_ok || src.id() == ID_allocate)
  {
    auto r = replacement_map.find(src);
    if(r == replacement_map.end())
    {
      auto counter = ++counters[src.id()];
      auto s =
        symbol_exprt(src.id_string() + std::to_string(counter), src.type());
      replacement_map.emplace(src, s);

      if(src.id() == ID_state_is_cstring)
      {
        std::cout << "R " << format(s) << " --> " << format(src) << '\n';
      }

      return std::move(s);
    }
    else
      return r->second;
  }

  for(auto &op : src.operands())
    op = replace(op);

  return src;
}

void axiomst::node(const exprt &src)
{
  if(src.id() == ID_state_is_cstring)
  {
    auto &is_cstring_expr = to_state_is_cstring_expr(src);
    is_cstring_exprs.insert(is_cstring_expr);

    {
      // is_cstring(ς, p) ⇒ r_ok(ς, p, 1)
      auto ok_expr = ternary_exprt(
        ID_state_r_ok,
        is_cstring_expr.state(),
        is_cstring_expr.address(),
        from_integer(1, size_type()),
        bool_typet());

      auto instance1 = replace(implies_exprt(src, ok_expr));
      std::cout << "AXIOMa1: " << format(instance1) << "\n";
      dest << instance1;

      auto ok_simplified = simplify_state_expr(ok_expr, address_taken, ns);
      ok_simplified.visit_pre([this](const exprt &src) { node(src); });
      auto instance2 = replace(implies_exprt(src, ok_simplified));
      std::cout << "AXIOMa2: " << format(instance2) << "\n";
      dest << instance2;
    }

    {
      // is_cstring(ς, p) --> is_cstring(ς, p + 1) ∨ ς(p)=0
      auto state = is_cstring_expr.state();
      auto p = is_cstring_expr.address();
      auto one = from_integer(1, signed_size_type());
      auto p_plus_one = plus_exprt(p, one, is_cstring_expr.op1().type());
      auto is_cstring_plus_one = state_is_cstring_exprt(state, p_plus_one);
      auto char_type = to_pointer_type(p.type()).base_type();
      auto zero = from_integer(0, char_type);
      auto star_p = evaluate_exprt(state, p, char_type);
      auto is_zero = equal_exprt(star_p, zero);
      auto instance =
        replace(implies_exprt(src, or_exprt(is_cstring_plus_one, is_zero)));
      std::cout << "AXIOMb: " << format(instance) << "\n";
      dest << instance;
      evaluate_exprs.insert(star_p);
      is_cstring_exprs.insert(is_cstring_plus_one);
    }
  }
  else if(src.id() == ID_evaluate)
  {
    const auto &evaluate_expr = to_evaluate_expr(src);
    evaluate_exprs.insert(evaluate_expr);
  }
  else if(src.id() == ID_state_live_object)
  {
    const auto &live_object_expr = to_state_live_object_expr(src);
    live_object_exprs.insert(live_object_expr);

    {
      // live_object(ς, p) --> p!=0
      auto instance = replace(implies_exprt(
        src, not_exprt(null_pointer(live_object_expr.address()))));
      std::cout << "AXIOMc: " << format(instance) << "\n";
      dest << instance;
    }
  }
  else if(src.id() == ID_state_is_dynamic_object)
  {
    const auto &is_dynamic_object_expr = to_state_is_dynamic_object_expr(src);
    is_dynamic_object_exprs.insert(is_dynamic_object_expr);
  }
  else if(src.id() == ID_allocate)
  {
    const auto &allocate_expr = to_allocate_expr(src);

    // May need to consider failure.
    // live_object(ς, allocate(ς, s))
    auto live_object_expr =
      state_live_object_exprt(allocate_expr.state(), allocate_expr);
    live_object_exprs.insert(live_object_expr);
    auto instance1 = replace(live_object_expr);
    std::cout << "ALLOCATE1: " << format(instance1) << "\n";
    dest << instance1;

    // object_size(ς, allocate(ς, s)) = s
    auto object_size_expr = state_object_size_exprt(
      allocate_expr.state(), allocate_expr, allocate_expr.size().type());
    object_size_exprs.insert(object_size_expr);
    auto instance2 =
      replace(equal_exprt(object_size_expr, allocate_expr.size()));
    std::cout << "ALLOCATE2: " << format(instance2) << "\n";
    dest << instance2;

    // pointer_offset(allocate(ς, s)) = 0
    auto pointer_offset_expr = pointer_offset(allocate_expr);
    //pointer_offset_exprs.insert(pointer_offset_expr);
    auto instance3 = replace(equal_exprt(
      pointer_offset_expr, from_integer(0, pointer_offset_expr.type())));
    std::cout << "ALLOCATE3: " << format(instance3) << "\n";
    dest << instance3;

    // is_dynamic_object(ς, allocate(ς, s))
    auto is_dynamic_object_expr =
      state_is_dynamic_object_exprt(allocate_expr.state(), allocate_expr);
    is_dynamic_object_exprs.insert(is_dynamic_object_expr);
    auto instance4 = replace(is_dynamic_object_expr);
    std::cout << "ALLOCATE4: " << format(instance4) << "\n";
    dest << instance4;
  }
  else if(src.id() == ID_deallocate_state)
  {
#if 0
    // Disabled since any other thread may have reclaimed
    // the memory.
    const auto &deallocate_state_expr = to_deallocate_state_expr(src);

    // ¬live_object(deallocate(ς, p), p)
    auto live_object_expr = state_live_object_exprt(
      deallocate_state_expr, deallocate_state_expr.address());
    live_object_exprs.insert(live_object_expr);
    auto instance1 = replace(not_exprt(live_object_expr));
    std::cout << "DEALLOCATE1: " << format(instance1) << "\n";
    dest << instance1;
#endif
  }
  else if(src.id() == ID_address_of)
  {
    address_of_exprs.insert(to_address_of_expr(src));
  }
  else if(src.id() == ID_state_object_size)
  {
    const auto &object_size_expr = to_state_object_size_expr(src);
    object_size_exprs.insert(object_size_expr);
  }
  else if(
    src.id() == ID_state_r_ok || src.id() == ID_state_w_ok ||
    src.id() == ID_state_rw_ok)
  {
    const auto &ok_expr = to_state_ok_expr(src);
    ok_exprs.insert(ok_expr);

    const auto &state = ok_expr.state();
    const auto &pointer = ok_expr.address();
    const auto &size = ok_expr.size();

    {
      // X_ok(p, s) <-->
      //   live_object(p)
      // ∧ offset(p)+s≤object_size(p)
      // ∧ !overflow(+, offset(p)+s)
      auto live_object = state_live_object_exprt(state, pointer);
      live_object_exprs.insert(live_object);
      auto live_object_simplified =
        simplify_state_expr_node(live_object, address_taken, ns);

      auto ssize_type = signed_size_type();
      auto offset = pointer_offset_exprt(pointer, ssize_type);
      auto offset_simplified =
        simplify_state_expr_node(offset, address_taken, ns);
      //      auto lower = binary_relation_exprt(
      //        offset_simplified, ID_ge, from_integer(0, ssize_type));

      auto size_type = ::size_type();
      auto object_size = state_object_size_exprt(state, pointer, size_type);
      object_size_exprs.insert(object_size);
      auto offset_casted =
        typecast_exprt::conditional_cast(offset_simplified, size_type);
      auto size_casted = typecast_exprt::conditional_cast(size, size_type);
      auto upper = binary_relation_exprt(
        plus_exprt(offset_casted, size_casted), ID_le, object_size);

      auto no_overflow = not_exprt(
        binary_overflow_exprt(offset_casted, ID_overflow_plus, size_casted));

      auto conjunction = and_exprt(live_object_simplified, upper, no_overflow);

      auto instance = replace(equal_exprt(src, conjunction));

      std::cout << "AXIOMd: " << format(instance) << "\n";
      dest << instance;
    }

    {
      // X_ok(ς, p) --> p!=0
      auto instance =
        replace(implies_exprt(src, not_exprt(null_pointer(pointer))));
      std::cout << "AXIOMe: " << format(instance) << "\n";
      dest << instance;
    }
  }
}

void axiomst::emit()
{
  for(auto &constraint : constraints)
  {
    constraint.visit_pre([this](const exprt &src) { node(src); });

    auto constraint_replaced = replace(constraint);
    std::cout << "CONSTRAINT: " << format(constraint_replaced) << "\n";
    dest << constraint_replaced;
  }

  object_size();

  // functional consistency is done last
  evaluate_fc();
  is_cstring_fc();
  ok_fc();
  live_object_fc();
  object_size_fc();
  is_dynamic_object_fc();
}
