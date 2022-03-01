/*******************************************************************\

Module: Instrument Contracts

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

/// \file
/// Instrument Contracts

#include "instrument_contracts.h"

#include <util/c_types.h>
#include <util/format_expr.h>
#include <util/prefix.h>
#include <util/replace_symbol.h>
#include <util/std_code.h>

#include <goto-programs/goto_model.h>

#include <iostream>

static exprt make_address(exprt src)
{
  if(src.id() == ID_dereference)
  {
    return to_dereference_expr(src).pointer();
  }
  else
    return address_of_exprt(src);
}

static bool is_symbol_member(const exprt &expr)
{
  if(expr.id() == ID_symbol)
    return true;
  else if(expr.id() == ID_member)
    return is_symbol_member(to_member_expr(expr).struct_op());
  else
    return false;
}

static exprt
make_assigns_assertion(const exprt::operandst &assigns, const exprt &lhs)
{
  // trivial?
  if(is_symbol_member(lhs))
  {
    for(auto &a : assigns)
      if(lhs == a)
        return true_exprt();
  }

  exprt::operandst disjuncts;

  for(auto &a : assigns)
  {
    auto a_address = make_address(a);
    auto lhs_address = make_address(lhs);
    lhs_address =
      typecast_exprt::conditional_cast(lhs_address, a_address.type());
    disjuncts.push_back(equal_exprt(a_address, lhs_address));
  }

  return disjunction(disjuncts);
}

static bool
is_procedure_local(const irep_idt &function_identifier, const exprt &lhs)
{
  if(lhs.id() == ID_member)
    return is_procedure_local(
      function_identifier, to_member_expr(lhs).struct_op());
  else if(lhs.id() == ID_index)
    return is_procedure_local(function_identifier, to_index_expr(lhs).array());
  else if(lhs.id() == ID_symbol)
  {
    const auto &symbol_expr = to_symbol_expr(lhs);
    return has_prefix(
      id2string(symbol_expr.get_identifier()),
      id2string(function_identifier) + "::");
  }
  else
    return false;
}

void instrument_contract_checks(
  goto_functionst::function_mapt::value_type &f,
  const namespacet &ns)
{
  auto &symbol = ns.lookup(f.first);
  auto &contract = to_code_with_contract_type(symbol.type);
  auto &body = f.second.body;

  if(body.instructions.empty())
    return; // nothing to check

  // precondition?
  if(!contract.requires().empty())
  {
    // stick these in as assumptions
    for(auto &assumption : contract.requires())
    {
      body.insert_before(
        body.instructions.begin(),
        goto_programt::make_assumption(
          assumption, assumption.source_location()));
    }
  }

  // postcondition?
  if(!contract.ensures().empty())
  {
    // stick these in as assertions at the end
    auto last = body.instructions.end();
    if(std::prev(last)->is_end_function())
      last = std::prev(last);
    for(auto &assertion : contract.ensures())
    {
      auto location = assertion.source_location();
      location.set_function(f.first); // seems to be missing
      location.set_property_class(ID_postcondition);
      location.set_comment("postcondition");
      auto assertion_instruction =
        goto_programt::make_assertion(assertion, std::move(location));
      body.insert_before_swap(last, assertion_instruction);
    }
  }

  // assigns?
  if(!contract.assigns().empty() || !contract.ensures().empty())
  {
    for(auto it = body.instructions.begin(); it != body.instructions.end();
        it++)
    {
      if(it->is_assign())
      {
        const auto &lhs = it->assign_lhs();

        // Parameter or local? Ignore.
        if(is_procedure_local(f.first, lhs))
          continue; // ok

        // maybe not ok
        auto assigns_assertion =
          make_assigns_assertion(contract.assigns(), lhs);
        auto location = it->source_location();
        location.set_property_class("assigns");
        location.set_comment("assigns clause");
        auto assertion_instruction = goto_programt::make_assertion(
          std::move(assigns_assertion), std::move(location));
        body.insert_before_swap(it, assertion_instruction);
        it++; // skip over the assertion we have just generated
      }
    }
  }
}

void replace_function_calls_by_contracts(
  goto_functionst::function_mapt::value_type &f,
  const goto_modelt &goto_model)
{
  auto &body = f.second.body;
  const namespacet ns(goto_model.symbol_table);

  for(auto it = body.instructions.begin(); it != body.instructions.end(); it++)
  {
    if(it->is_function_call())
    {
      const auto &function = it->call_function();
      if(function.id() == ID_symbol)
      {
        const auto &symbol = ns.lookup(to_symbol_expr(function));

        auto &contract = to_code_with_contract_type(symbol.type);

        if(
          contract.requires().empty() && contract.ensures().empty() &&
          contract.assigns().empty())
          continue;

        // need to substitute parameters
        const auto f_it =
          goto_model.goto_functions.function_map.find(symbol.name);
        if(f_it == goto_model.goto_functions.function_map.end())
          DATA_INVARIANT(false, "failed to find function in function_map");

        replace_symbolt replace_symbol;
        const auto &parameters = to_code_type(symbol.type).parameters();
        const auto &arguments = it->call_arguments();

        for(std::size_t p = 0; p < f_it->second.parameter_identifiers.size();
            p++)
        {
          auto p_symbol = symbol_exprt(
            f_it->second.parameter_identifiers[p], parameters[p].type());
          replace_symbol.insert(p_symbol, arguments[p]);
        }

        for(auto &precondition : contract.requires())
        {
          auto location = it->source_location();
          location.set_property_class(ID_precondition);
          location.set_comment(
            "precondition " + id2string(symbol.display_name()));

          auto replaced_precondition = precondition;
          replace_symbol(replaced_precondition);

          auto assertion_instruction =
            goto_programt::make_assertion(replaced_precondition, location);

          body.insert_before_swap(it, assertion_instruction);
        }

        // advance the iterator
        it = std::next(it, contract.requires().size());

        // havoc the 'assigned' variables
        for(auto &lhs : contract.assigns())
        {
          auto location = it->source_location();

          auto rhs = side_effect_expr_nondett(lhs.type(), location);
          auto replaced_lhs = lhs;
          replace_symbol(replaced_lhs);

          auto assignment_instruction =
            goto_programt::make_assignment(replaced_lhs, rhs, location);

          body.insert_before_swap(it, assignment_instruction);
        }

        // advance the iterator
        it = std::next(it, contract.assigns().size());

        // assume the postconditions
        for(auto &postcondition : contract.ensures())
        {
          auto &location = it->source_location();

          auto replaced_postcondition = postcondition;
          replace_symbol(replaced_postcondition);

          auto assumption_instruction =
            goto_programt::make_assumption(replaced_postcondition, location);

          body.insert_before_swap(it, assumption_instruction);
        }

        // advance the iterator
        it = std::next(it, contract.ensures().size());

        // remove the function call
        it->turn_into_skip();
      }
    }
  }
}

void instrument_contracts(goto_modelt &goto_model)
{
  const namespacet ns(goto_model.symbol_table);

  for(auto &f : goto_model.goto_functions.function_map)
  {
    instrument_contract_checks(f, ns);
    replace_function_calls_by_contracts(f, goto_model);
  }
}
