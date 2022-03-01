/*******************************************************************\

Module: State Encoding

Author:

\*******************************************************************/

#include "state_encoding.h"

#include <util/c_types.h>
#include <util/format_expr.h>
#include <util/mathematical_expr.h>
#include <util/pointer_expr.h>
#include <util/prefix.h>
#include <util/std_code.h>

#include <goto-programs/goto_model.h>

#include "equality_propagation.h"
#include "solver.h"
#include "state.h"
#include "state_encoding_targets.h"
#include "variable_encoding.h"

#include <iostream>

class state_encodingt
{
public:
  explicit state_encodingt(const goto_functionst &__goto_functions)
    : goto_functions(__goto_functions)
  {
  }

  void operator()(
    const goto_functionst::function_mapt::const_iterator,
    encoding_targett &);

  void encode(
    const goto_functiont &,
    const std::string &state_prefix,
    const std::string &annotation,
    const symbol_exprt &entry_state,
    const exprt &return_lhs,
    encoding_targett &);

protected:
  using loct = goto_programt::const_targett;
  const goto_functionst &goto_functions;

  symbol_exprt out_state_expr(loct) const;
  symbol_exprt state_expr_with_suffix(loct, const std::string &suffix) const;
  symbol_exprt out_state_expr(loct, bool taken) const;
  symbol_exprt in_state_expr(loct) const;
  std::vector<symbol_exprt> incoming_symbols(loct) const;
  exprt evaluate_expr(loct, const exprt &, const exprt &) const;
  exprt evaluate_expr_rec(
    loct,
    const exprt &,
    const exprt &,
    const std::unordered_set<symbol_exprt, irep_hash> &) const;
  exprt replace_nondet_rec(
    loct,
    const exprt &,
    std::vector<symbol_exprt> &nondet_symbols) const;
  exprt evaluate_expr(loct, const exprt &) const;
  exprt address_rec(loct, const exprt &, exprt) const;
  static exprt state_lambda_expr(exprt);
  exprt forall_states_expr(loct, exprt) const;
  exprt forall_states_expr(loct, exprt, exprt) const;
  void setup_incoming(const goto_functiont &);
  exprt assignment_constraint(loct, exprt lhs, exprt rhs) const;
  void function_call(goto_programt::const_targett, encoding_targett &);
  void function_call_symbol(goto_programt::const_targett, encoding_targett &);

  std::string state_prefix;
  std::string annotation;
  loct first_loc;
  symbol_exprt entry_state = symbol_exprt(irep_idt(), typet());
  exprt return_lhs = nil_exprt();
  using incomingt = std::map<loct, std::vector<loct>>;
  incomingt incoming;
};

symbol_exprt state_encodingt::out_state_expr(loct loc) const
{
  return state_expr_with_suffix(loc, "");
}

symbol_exprt state_encodingt::state_expr_with_suffix(
  loct loc,
  const std::string &suffix) const
{
  irep_idt identifier =
    state_prefix + std::to_string(loc->location_number) + suffix;
  return symbol_exprt(identifier, state_predicate_type());
}

symbol_exprt state_encodingt::out_state_expr(loct loc, bool taken) const
{
  return state_expr_with_suffix(loc, taken ? "T" : "");
}

std::vector<symbol_exprt> state_encodingt::incoming_symbols(loct loc) const
{
  auto incoming_it = incoming.find(loc);

  DATA_INVARIANT(incoming_it != incoming.end(), "incoming is complete");

  std::vector<symbol_exprt> symbols;
  symbols.reserve(incoming_it->second.size());

  for(auto &loc_in : incoming_it->second)
  {
    std::string suffix;

    // conditional jump from loc_in to loc?
    if(
      loc_in->is_goto() && !loc_in->condition().is_true() &&
      loc != std::next(loc_in))
    {
      suffix = "T";
    }

    symbols.push_back(state_expr_with_suffix(loc_in, suffix));
  }

  return symbols;
}

symbol_exprt state_encodingt::in_state_expr(loct loc) const
{
  if(loc == first_loc)
    return entry_state;

  auto incoming_symbols = this->incoming_symbols(loc);

  if(incoming_symbols.size() == 1)
    return incoming_symbols.front();
  else
    return state_expr_with_suffix(loc, "in");
}

exprt state_encodingt::evaluate_expr(
  loct loc,
  const exprt &state,
  const exprt &what) const
{
  return evaluate_expr_rec(loc, state, what, {});
}

exprt state_encodingt::replace_nondet_rec(
  loct loc,
  const exprt &what,
  std::vector<symbol_exprt> &nondet_symbols) const
{
  if(what.id() == ID_side_effect)
  {
    auto &side_effect = to_side_effect_expr(what);
    auto statement = side_effect.get_statement();
    if(statement == ID_nondet)
    {
      irep_idt identifier =
        "nondet::" + state_prefix + std::to_string(loc->location_number);
      auto symbol = symbol_exprt(identifier, side_effect.type());
      nondet_symbols.push_back(symbol);
      return std::move(symbol);
    }
    else
      return what; // leave it
  }
  else
  {
    exprt tmp = what;
    for(auto &op : tmp.operands())
      op = replace_nondet_rec(loc, op, nondet_symbols);
    return tmp;
  }
}

exprt state_encodingt::evaluate_expr_rec(
  loct loc,
  const exprt &state,
  const exprt &what,
  const std::unordered_set<symbol_exprt, irep_hash> &bound_symbols) const
{
  PRECONDITION(state.type().id() == ID_state);

  if(what.id() == ID_symbol)
  {
    const auto &symbol_expr = to_symbol_expr(what);

    if(symbol_expr.get_identifier() == "__CPROVER_return_value")
    {
      auto new_symbol = symbol_exprt("return_value", what.type());
      return evaluate_exprt(
        state, address_rec(loc, state, new_symbol), what.type());
    }
    else if(bound_symbols.find(symbol_expr) == bound_symbols.end())
    {
      return evaluate_exprt(state, address_rec(loc, state, what), what.type());
    }
    else
      return what; // leave as is
  }
  else if(
    what.id() == ID_dereference || what.id() == ID_member ||
    what.id() == ID_index)
  {
    return evaluate_exprt(state, address_rec(loc, state, what), what.type());
  }
  else if(what.id() == ID_forall || what.id() == ID_exists)
  {
    auto new_quantifier_expr = to_quantifier_expr(what); // copy
    auto new_bound_symbols = bound_symbols;              // copy

    for(const auto &v : new_quantifier_expr.variables())
      new_bound_symbols.insert(v);

    new_quantifier_expr.where() = evaluate_expr_rec(
      loc, state, new_quantifier_expr.where(), new_bound_symbols);

    return std::move(new_quantifier_expr);
  }
  else if(what.id() == ID_address_of)
  {
    const auto &object = to_address_of_expr(what).object();
    return address_rec(loc, state, object);
  }
  else if(what.id() == ID_live_object)
  {
    const auto &live_object_expr = to_live_object_expr(what);
    auto pointer =
      evaluate_expr_rec(loc, state, live_object_expr.pointer(), bound_symbols);
    return state_live_object_exprt(state, pointer);
  }
  else if(what.id() == ID_is_dynamic_object)
  {
    const auto &is_dynamic_object_expr = to_is_dynamic_object_expr(what);
    auto pointer = evaluate_expr_rec(
      loc, state, is_dynamic_object_expr.address(), bound_symbols);
    return state_is_dynamic_object_exprt(state, pointer);
  }
  else if(what.id() == ID_object_size)
  {
    const auto &object_size_expr = to_object_size_expr(what);
    auto pointer =
      evaluate_expr_rec(loc, state, object_size_expr.pointer(), bound_symbols);
    return state_object_size_exprt(state, pointer, what.type());
  }
  else if(what.id() == ID_r_ok || what.id() == ID_w_ok || what.id() == ID_rw_ok)
  {
    // we need to add the state
    const auto &r_or_w_ok_expr = to_r_or_w_ok_expr(what);
    auto pointer =
      evaluate_expr_rec(loc, state, r_or_w_ok_expr.pointer(), bound_symbols);
    auto size =
      evaluate_expr_rec(loc, state, r_or_w_ok_expr.size(), bound_symbols);
    auto new_id = what.id() == ID_r_ok   ? ID_state_r_ok
                  : what.id() == ID_w_ok ? ID_state_w_ok
                                         : ID_state_rw_ok;
    return ternary_exprt(new_id, state, pointer, size, what.type());
  }
  else if(what.id() == ID_is_cstring)
  {
    // we need to add the state
    const auto &is_cstring_expr = to_unary_expr(what);
    auto pointer =
      evaluate_expr_rec(loc, state, is_cstring_expr.op(), bound_symbols);
    return binary_predicate_exprt(state, ID_state_is_cstring, pointer);
  }
  else
  {
    exprt tmp = what;
    for(auto &op : tmp.operands())
      op = evaluate_expr_rec(loc, state, op, bound_symbols);
    return tmp;
  }
}

exprt state_encodingt::evaluate_expr(loct loc, const exprt &what) const
{
  return evaluate_expr(loc, in_state_expr(loc), what);
}

exprt state_encodingt::state_lambda_expr(exprt what)
{
  return lambda_exprt({state_expr()}, std::move(what));
}

exprt state_encodingt::forall_states_expr(loct loc, exprt what) const
{
  return forall_exprt(
    {state_expr()},
    implies_exprt(
      function_application_exprt(in_state_expr(loc), {state_expr()}),
      std::move(what)));
}

exprt state_encodingt::forall_states_expr(loct loc, exprt condition, exprt what)
  const
{
  return forall_exprt(
    {state_expr()},
    implies_exprt(
      and_exprt(
        function_application_exprt(in_state_expr(loc), {state_expr()}),
        std::move(condition)),
      std::move(what)));
}

exprt state_encodingt::address_rec(loct loc, const exprt &state, exprt expr)
  const
{
  if(expr.id() == ID_symbol)
  {
    return object_address_exprt(to_symbol_expr(expr));
  }
  else if(expr.id() == ID_member)
  {
    auto compound = to_member_expr(expr).struct_op();
    auto compound_address = address_rec(loc, state, std::move(compound));
    auto component_name = to_member_expr(expr).get_component_name();

    if(expr.type().id() == ID_array)
    {
      const auto &element_type = to_array_type(expr.type()).element_type();
      return field_address_exprt(
        std::move(compound_address),
        component_name,
        pointer_type(element_type));
    }
    else
    {
      return field_address_exprt(
        std::move(compound_address), component_name, pointer_type(expr.type()));
    }
  }
  else if(expr.id() == ID_index)
  {
    auto array = to_index_expr(expr).array();
    auto index_evaluated =
      evaluate_expr(loc, state, to_index_expr(expr).index());
    auto array_address = address_rec(loc, state, std::move(array));
    // The type of the element pointer may not be the type
    // of the base pointer, which may be a pointer to an array.
    return element_address_exprt(
      std::move(array_address),
      std::move(index_evaluated),
      pointer_type(expr.type()));
  }
  else if(expr.id() == ID_dereference)
    return evaluate_expr(loc, state, to_dereference_expr(expr).pointer());
  else if(expr.id() == ID_string_constant)
  {
    // we'll stick to 'address_of' here.
    return address_of_exprt(
      expr, pointer_type(to_array_type(expr.type()).element_type()));
  }
  else if(expr.id() == ID_array)
  {
    // TBD.
    throw incorrect_goto_program_exceptiont(
      "can't do array literals", expr.source_location());
  }
  else if(expr.id() == ID_union)
  {
    // TBD.
    throw incorrect_goto_program_exceptiont(
      "can't do union literals", expr.source_location());
  }
  else
    return nil_exprt();
}

exprt state_encodingt::assignment_constraint(loct loc, exprt lhs, exprt rhs)
  const
{
  auto s = state_expr();

  auto address = address_rec(loc, s, lhs);

  exprt rhs_evaluated = evaluate_expr(loc, s, rhs);

  std::vector<symbol_exprt> nondet_symbols;
  exprt new_value = replace_nondet_rec(loc, rhs_evaluated, nondet_symbols);

  auto new_state = update_state_exprt(s, address, new_value);

  return forall_states_expr(
    loc, function_application_exprt(out_state_expr(loc), {new_state}));
}

void state_encodingt::setup_incoming(const goto_functiont &goto_function)
{
  forall_goto_program_instructions(it, goto_function.body)
    incoming[it];

  forall_goto_program_instructions(it, goto_function.body)
  {
    if(it->is_goto())
      incoming[it->get_target()].push_back(it);
  }

  forall_goto_program_instructions(it, goto_function.body)
  {
    auto next = std::next(it);
    if(it->is_goto() && it->condition().is_true())
    {
    }
    else if(next != goto_function.body.instructions.end())
    {
      incoming[next].push_back(it);
    }
  }
}

static exprt simplifying_not(exprt src)
{
  if(src.id() == ID_not)
    return to_not_expr(src).op();
  else
    return not_exprt(src);
}

static bool has_contract(const code_with_contract_typet &contract)
{
  return !contract.assigns().empty() || !contract.requires().empty() ||
         !contract.ensures().empty();
}

void state_encodingt::function_call_symbol(
  goto_programt::const_targett loc,
  encoding_targett &dest)
{
  const auto &function = to_symbol_expr(loc->call_function());
  const auto &type = to_code_type(function.type());
  auto identifier = function.get_identifier();

  auto new_annotation = annotation + u8" \u2192 " + id2string(identifier);
  dest.annotation(new_annotation);

  // malloc is special-cased
  if(identifier == "malloc")
  {
    auto state = state_expr();
    PRECONDITION(loc->call_arguments().size() == 1);
    auto size_evaluated = evaluate_expr(loc, state, loc->call_arguments()[0]);

    auto lhs_address = address_rec(loc, state, loc->call_lhs());
    auto lhs_type = to_pointer_type(lhs_address.type());
    exprt new_state = update_state_exprt(
      state, lhs_address, allocate_exprt(state, size_evaluated, lhs_type));
    dest << forall_states_expr(
      loc, function_application_exprt(out_state_expr(loc), {new_state}));
    return;
  }

  // free is special-cased
  if(identifier == "free")
  {
    auto state = state_expr();
    PRECONDITION(loc->call_arguments().size() == 1);
    auto address_evaluated =
      evaluate_expr(loc, state, loc->call_arguments()[0]);

    exprt new_state = deallocate_state_exprt(state, address_evaluated);
    dest << forall_states_expr(
      loc, function_application_exprt(out_state_expr(loc), {new_state}));
    return;
  }

  // Find the function
  auto f = goto_functions.function_map.find(identifier);
  if(f == goto_functions.function_map.end())
    DATA_INVARIANT(false, "failed find function in function_map");

  // Do we have a function body?
  if(!f->second.body_available())
  {
    // no function body -- do LHS assignment nondeterministically, if any
    if(loc->call_lhs().is_not_nil())
    {
      auto rhs = side_effect_expr_nondett(
        loc->call_lhs().type(), loc->source_location());
      dest << assignment_constraint(loc, loc->call_lhs(), std::move(rhs));
    }
    else
    {
      // This is a SKIP.
      dest << equal_exprt(out_state_expr(loc), in_state_expr(loc));
    }
  }

  // Evaluate the arguments of the call in the 'in state'
  exprt arguments_state = state_expr();

  for(std::size_t i = 0; i < type.parameters().size(); i++)
  {
    auto address = object_address_exprt(symbol_exprt(
      f->second.parameter_identifiers[i], type.parameters()[i].type()));
    auto argument = loc->call_arguments()[i];
    auto value = evaluate_expr(loc, state_expr(), argument);
    arguments_state = update_state_exprt(arguments_state, address, value);
  }

  // Now assign them
  auto function_entry_state = state_expr_with_suffix(loc, "Entry");
  dest << forall_states_expr(
    loc, function_application_exprt(function_entry_state, {arguments_state}));

  // now do the body, recursively
  state_encodingt body_state_encoding(goto_functions);
  auto new_state_prefix =
    state_prefix + std::to_string(loc->location_number) + ".";
  body_state_encoding.encode(
    f->second,
    new_state_prefix,
    new_annotation,
    function_entry_state,
    nil_exprt(),
    dest);

  // exit state of called function
  auto exit_loc = std::prev(f->second.body.instructions.end());
  irep_idt exit_state_identifier =
    new_state_prefix + std::to_string(exit_loc->location_number);
  auto exit_state = symbol_exprt(exit_state_identifier, state_predicate_type());

  // now link up return state
  dest << equal_exprt(out_state_expr(loc), std::move(exit_state));
}

void state_encodingt::function_call(
  goto_programt::const_targett loc,
  encoding_targett &dest)
{
  // Function pointer?
  const auto &function = loc->call_function();
  if(function.id() == ID_dereference)
  {
    // TBD.
    throw incorrect_goto_program_exceptiont(
      "can't do function pointers", loc->source_location());
  }
  else if(function.id() == ID_symbol)
  {
    function_call_symbol(loc, dest);
  }
  else
  {
    DATA_INVARIANT(
      false, "got function that's neither a symbol nor a function pointer");
  }
}

void state_encodingt::operator()(
  goto_functionst::function_mapt::const_iterator f_entry,
  encoding_targett &dest)
{
  const auto &goto_function = f_entry->second;

  if(goto_function.body.instructions.empty())
    return;

  // initial state
  auto in_state = symbol_exprt("SInitial", state_predicate_type());

  dest << forall_exprt(
    {state_expr()},
    implies_exprt(
      true_exprt(), function_application_exprt(in_state, {state_expr()})));

  auto annotation = id2string(f_entry->first);

  encode(goto_function, "S", annotation, in_state, nil_exprt(), dest);
}

void state_encodingt::encode(
  const goto_functiont &goto_function,
  const std::string &state_prefix,
  const std::string &annotation,
  const symbol_exprt &entry_state,
  const exprt &return_lhs,
  encoding_targett &dest)
{
  first_loc = goto_function.body.instructions.begin();
  this->state_prefix = state_prefix;
  this->annotation = annotation;
  this->entry_state = entry_state;
  this->return_lhs = return_lhs;

  setup_incoming(goto_function);

  // constraints for each instruction
  forall_goto_program_instructions(loc, goto_function.body)
  {
    // pass on the source code location
    dest.set_source_location(loc->source_location());

    // constraints on the incoming state
    {
      auto incoming_symbols = this->incoming_symbols(loc);

      if(incoming_symbols.size() >= 2)
      {
        auto s = state_expr();
        for(auto incoming_symbol : incoming_symbols)
        {
          dest << forall_exprt(
            {s},
            implies_exprt(
              function_application_exprt(incoming_symbol, {s}),
              function_application_exprt(in_state_expr(loc), {s})));
        }
      }
    }

    if(loc->is_assign())
    {
      auto &lhs = loc->assign_lhs();
      auto &rhs = loc->assign_rhs();

      if(lhs.type().id() == ID_array)
      {
        // skip
        dest << equal_exprt(out_state_expr(loc), in_state_expr(loc));
      }
      else if(lhs.type().id() == ID_struct_tag)
      {
        // skip
        dest << equal_exprt(out_state_expr(loc), in_state_expr(loc));
      }
      else if(
        lhs.id() == ID_symbol &&
        has_prefix(
          id2string(to_symbol_expr(lhs).get_identifier()), CPROVER_PREFIX) &&
        to_symbol_expr(lhs).get_identifier() != CPROVER_PREFIX "rounding_mode")
      {
        // skip for now
        dest << equal_exprt(out_state_expr(loc), in_state_expr(loc));
      }
      else
        dest << assignment_constraint(loc, lhs, rhs);
    }
    else if(loc->is_assume())
    {
      // we produce ∅ when the assumption is false
      auto state = state_expr();
      auto condition_evaluated = evaluate_expr(loc, state, loc->condition());

      dest << forall_states_expr(
        loc,
        condition_evaluated,
        function_application_exprt(out_state_expr(loc), {state}));
    }
    else if(loc->is_goto())
    {
      // We produce ∅ when the 'other' branch is taken. Get the condition.
      const auto &condition = loc->condition();

      if(condition.is_true())
      {
        dest << equal_exprt(out_state_expr(loc), in_state_expr(loc));
      }
      else
      {
        auto state = state_expr();
        auto condition_evaluated = evaluate_expr(loc, state, condition);

        dest << forall_states_expr(
          loc,
          condition_evaluated,
          function_application_exprt(out_state_expr(loc, true), {state}));

        dest << forall_states_expr(
          loc,
          simplifying_not(condition_evaluated),
          function_application_exprt(out_state_expr(loc, false), {state}));
      }
    }
    else if(loc->is_assert())
    {
      // all assertions need to hold
      dest << forall_states_expr(
        loc, evaluate_expr(loc, state_expr(), loc->condition()));

      dest << equal_exprt(out_state_expr(loc), in_state_expr(loc));
    }
    else if(
      loc->is_skip() || loc->is_assert() || loc->is_location() ||
      loc->is_end_function() || loc->is_other())
    {
      dest << equal_exprt(out_state_expr(loc), in_state_expr(loc));
    }
    else if(loc->is_decl() || loc->is_dead())
    {
      // may wish to havoc the symbol
      dest << equal_exprt(out_state_expr(loc), in_state_expr(loc));
    }
    else if(loc->is_function_call())
    {
      function_call(loc, dest);
    }
    else if(loc->is_set_return_value())
    {
      const auto &rhs = loc->return_value();

      if(return_lhs.is_nil())
      {
        // treat these as assignments to a special symbol named 'return_value'
        auto lhs = symbol_exprt("return_value", rhs.type());
        dest << assignment_constraint(loc, std::move(lhs), std::move(rhs));
      }
      else
      {
      }
    }
  }
}

void state_encoding(
  const goto_modelt &goto_model,
  bool program_is_inlined,
  encoding_targett &dest)
{
  if(program_is_inlined)
  {
    auto f_entry = goto_model.goto_functions.function_map.find(
      goto_functionst::entry_point());

    if(f_entry == goto_model.goto_functions.function_map.end())
      throw incorrect_goto_program_exceptiont("The program has no entry point");

    dest.annotation("function " + id2string(f_entry->first));

    state_encodingt{goto_model.goto_functions}(f_entry, dest);
  }
  else
  {
    // sort alphabetically
    const auto sorted = goto_model.goto_functions.sorted();
    const namespacet ns(goto_model.symbol_table);
    for(auto &f : sorted)
    {
      const auto &symbol = ns.lookup(f->first);
      if(
        f->first == goto_functionst::entry_point() ||
        has_contract(to_code_with_contract_type(symbol.type)))
      {
        dest.annotation("");
        dest.annotation("function " + id2string(f->first));
        state_encodingt{goto_model.goto_functions}(f, dest);
      }
    }
  }
}

void format_hooks();

void state_encoding(
  const goto_modelt &goto_model,
  state_encoding_formatt state_encoding_format,
  bool program_is_inlined,
  std::ostream &out)
{
  switch(state_encoding_format)
  {
  case state_encoding_formatt::ASCII:
  {
    format_hooks();
    ascii_encoding_targett dest(out);
    state_encoding(goto_model, program_is_inlined, dest);
  }
  break;

  case state_encoding_formatt::SMT2:
  {
    const namespacet ns(goto_model.symbol_table);
    smt2_encoding_targett dest(ns, out);
    state_encoding(goto_model, program_is_inlined, dest);
  }
  break;
  }
}

void variable_encoding(
  const goto_modelt &goto_model,
  state_encoding_formatt state_encoding_format,
  std::ostream &out)
{
  const namespacet ns(goto_model.symbol_table);

  format_hooks();

  container_encoding_targett container;
  state_encoding(goto_model, true, container);

  equality_propagation(container.constraints);

  variable_encoding(container.constraints);

  switch(state_encoding_format)
  {
  case state_encoding_formatt::ASCII:
  {
    ascii_encoding_targett dest(out);
    dest << container;
  }
  break;

  case state_encoding_formatt::SMT2:
  {
    smt2_encoding_targett dest(ns, out);
    dest << container;
  }
  break;
  }
}

solver_resultt state_encoding_solver(
  const goto_modelt &goto_model,
  bool program_is_inlined,
  std::size_t loop_limit)
{
  const namespacet ns(goto_model.symbol_table);

  format_hooks();

  container_encoding_targett container;
  state_encoding(goto_model, program_is_inlined, container);

  equality_propagation(container.constraints);

  ascii_encoding_targett dest(std::cout);
  dest << container;

  return solver(container.constraints, loop_limit, ns);
}
