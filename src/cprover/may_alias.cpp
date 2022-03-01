/*******************************************************************\

Module: Solver

Author:

\*******************************************************************/

/// \file
/// May Alias

#include "may_alias.h"

#include <util/c_types.h>
#include <util/format_expr.h>
#include <util/format_type.h>
#include <util/namespace.h>
#include <util/pointer_expr.h>
#include <util/std_expr.h>
#include <util/symbol.h>

#include <iostream>

bool permitted_by_strict_aliasing(const typet &a, const typet &b)
{
  // C99; ISO/IEC 9899:1999 6.5/7
  if(a == b)
    return true;
  else if(a == signed_char_type() || a == unsigned_char_type())
    return true; // char * can alias anyting
  else if(b == signed_char_type() || b == unsigned_char_type())
    return true; // char * can alias anyting
  else if(
    (a.id() == ID_unsignedbv || a.id() == ID_signedbv) &&
    (b.id() == ID_unsignedbv || b.id() == ID_signedbv))
  {
    // signed/unsigned of same width can alias
    return to_bitvector_type(a).get_width() == to_bitvector_type(b).get_width();
  }
  else
  {
    return false;
  }
}

bool is_object_field_element(const exprt &expr)
{
  if(expr.id() == ID_object_address)
    return true;
  else if(expr.id() == ID_element_address)
    return is_object_field_element(to_element_address_expr(expr).base());
  else if(expr.id() == ID_field_address)
    return is_object_field_element(to_field_address_expr(expr).base());
  else
    return false;
}

bool prefix_of(const typet &a, const typet &b, const namespacet &ns)
{
  if(a == b)
    return true;

  if(a.id() == ID_struct_tag)
    return prefix_of(ns.follow_tag(to_struct_tag_type(a)), b, ns);

  if(b.id() == ID_struct_tag)
    return prefix_of(a, ns.follow_tag(to_struct_tag_type(b)), ns);

  if(a.id() != ID_struct || b.id() != ID_struct)
    return false;

  const auto &a_struct = to_struct_type(a);
  const auto &b_struct = to_struct_type(b);

  return a_struct.is_prefix_of(b_struct) || b_struct.is_prefix_of(a_struct);
}

static optionalt<object_address_exprt> find_object(const exprt &expr)
{
  if(expr.id() == ID_object_address)
    return to_object_address_expr(expr);
  else if(expr.id() == ID_field_address)
    return find_object(to_field_address_expr(expr).base());
  else if(expr.id() == ID_element_address)
    return find_object(to_element_address_expr(expr).base());
  else
    return {};
}

// Is 'expr' on the stack and it's address is not taken?
static bool stack_and_not_dirty(
  const exprt &expr,
  const std::unordered_set<symbol_exprt, irep_hash> &address_taken,
  const namespacet &ns)
{
  auto object = find_object(expr);
  if(object.has_value())
  {
    auto symbol_expr = object->object_expr();
    const auto &symbol = ns.lookup(symbol_expr);
    return !symbol.is_static_lifetime &&
           address_taken.find(symbol_expr) == address_taken.end();
  }
  else
    return false;
}

optionalt<exprt> may_alias(
  const exprt &a,
  const exprt &b,
  const std::unordered_set<symbol_exprt, irep_hash> &address_taken,
  const namespacet &ns)
{
  PRECONDITION(a.type().id() == ID_pointer);
  PRECONDITION(b.type().id() == ID_pointer);

  static const auto true_expr = true_exprt();
  static const auto false_expr = false_exprt();

  // syntactically the same?
  if(a == b)
    return true_expr;

  // consider the strict aliasing rule
  const auto &a_base = to_pointer_type(a.type()).base_type();
  const auto &b_base = to_pointer_type(b.type()).base_type();

  if(!permitted_by_strict_aliasing(a_base, b_base))
  {
    // The object is known to be different, because of strict aliasing.
    return false_expr;
  }

  // a and b are both object/field/element?
  if(is_object_field_element(a) && is_object_field_element(b))
  {
    if(a.id() == ID_object_address && b.id() == ID_object_address)
    {
      if(
        to_object_address_expr(a).object_identifier() ==
        to_object_address_expr(b).object_identifier())
      {
        return true_expr; // the same
      }
      else
        return false_expr; // different
    }
    else if(a.id() == ID_element_address && b.id() == ID_element_address)
    {
      const auto &a_element_address = to_element_address_expr(a);
      const auto &b_element_address = to_element_address_expr(b);

      auto base_may_alias = may_alias(
        a_element_address.base(), b_element_address.base(), address_taken, ns);

      CHECK_RETURN(base_may_alias.has_value());

      if(base_may_alias->is_false())
        return false_expr;
      else
      {
        return and_exprt(
          *base_may_alias,
          equal_exprt(a_element_address.index(), b_element_address.index()));
      }
    }
    else if(a.id() == ID_field_address && b.id() == ID_field_address)
    {
      const auto &a_field_address = to_field_address_expr(a);
      const auto &b_field_address = to_field_address_expr(b);

      // structs can't alias unless one is a prefix of the other
      if(!prefix_of(
           a_field_address.type().base_type(),
           b_field_address.type().base_type(),
           ns))
      {
        return false_expr;
      }

      if(a_field_address.component_name() == b_field_address.component_name())
        return may_alias(
          a_field_address.base(), b_field_address.base(), address_taken, ns);
      else
        return false_expr;
    }
    else
      return false_expr;
  }

  // is one of them stack-allocated and it's address is not taken?
  if(stack_and_not_dirty(a, address_taken, ns))
    return false_expr; // can't alias

  if(stack_and_not_dirty(b, address_taken, ns))
    return false_expr; // can't alias

  // we don't know
  return {};
}
