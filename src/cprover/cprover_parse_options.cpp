/*******************************************************************\

Module: CPROVER Command Line Option Processing

Author: Daniel Kroening, dkr@amazon.com

\*******************************************************************/

/// \file
/// cprover Command Line Options Processing

#include "cprover_parse_options.h"

#include <util/config.h>
#include <util/cout_message.h>
#include <util/exit_codes.h>
#include <util/options.h>
#include <util/signal_catcher.h>
#include <util/unicode.h>
#include <util/version.h>

#include <goto-programs/adjust_float_expressions.h>
#include <goto-programs/goto_inline.h>
#include <goto-programs/initialize_goto_model.h>
#include <goto-programs/loop_ids.h>
#include <goto-programs/set_properties.h>
#include <goto-programs/show_goto_functions.h>

#include <ansi-c/ansi_c_language.h>
#include <langapi/mode.h>

#include "c_safety_checks.h"
#include "help_formatter.h"
#include "instrument_contracts.h"
#include "instrument_given_invariants.h"
#include "state_encoding.h"

#include <fstream>
#include <iostream>

static void show_goto_functions(const goto_modelt &goto_model)
{
  // sort alphabetically
  const auto sorted = goto_model.goto_functions.sorted();

  const namespacet ns(goto_model.symbol_table);
  for(const auto &fun : sorted)
  {
    const symbolt &symbol = ns.lookup(fun->first);
    const bool has_body = fun->second.body_available();

    if(has_body)
    {
      std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\n";

      std::cout << symbol.display_name() << " /* " << symbol.name << " */\n";
      fun->second.body.output(ns, symbol.name, std::cout);
    }
  }
}

int cprover_parse_optionst::main()
{
  try
  {
    install_signal_catcher();

    cmdlinet cmdline;

    if(cmdline.parse(argc, argv, CPROVER_OPTIONS))
    {
      std::cerr << "Usage error!" << '\n';
      return CPROVER_EXIT_USAGE_ERROR;
    }

    if(cmdline.isset("version"))
    {
      std::cout << CBMC_VERSION << '\n';
      return CPROVER_EXIT_SUCCESS;
    }

    if(cmdline.isset('?') || cmdline.isset("help") || cmdline.isset('h'))
    {
      help();
      return CPROVER_EXIT_SUCCESS;
    }

    register_language(new_ansi_c_language);

    if(cmdline.args.empty())
    {
      std::cerr << "Please provide a program to verify\n";
      return CPROVER_EXIT_INCORRECT_TASK;
    }

    bool perform_inlining = !cmdline.isset("no-inline");

    config.set(cmdline);
    console_message_handlert message_handler;

    optionst options;
    auto goto_model =
      initialize_goto_model(cmdline.args, message_handler, options);
    adjust_float_expressions(goto_model);
    instrument_given_invariants(goto_model);

    if(!perform_inlining)
      instrument_contracts(goto_model);

    if(cmdline.isset("safety"))
      c_safety_checks(goto_model);

    if(cmdline.isset("no-assertions"))
      no_assertions(goto_model);

    label_properties(goto_model);

    // bool perform_inlining = false;
    bool variable_encoding = cmdline.isset("variables");

    if(perform_inlining || variable_encoding)
    {
      std::cout << "Performing inlining\n";
      goto_inline(goto_model, message_handler, false);
    }

    goto_model.goto_functions.compute_target_numbers();
    goto_model.goto_functions.compute_location_numbers();

    if(cmdline.isset("show-goto-functions"))
    {
      show_goto_functions(goto_model);
      return CPROVER_EXIT_SUCCESS;
    }

    // show loop ids?
    if(cmdline.isset("show-loops"))
    {
      show_loop_ids(ui_message_handlert::uit::PLAIN, goto_model);
      return CPROVER_EXIT_SUCCESS;
    }

    if(cmdline.isset("validate-goto-model"))
    {
      goto_model.validate();
    }

    if(cmdline.isset("smt2") || cmdline.isset("text") || variable_encoding)
    {
      auto format = cmdline.isset("smt2") ? state_encoding_formatt::SMT2
                                          : state_encoding_formatt::ASCII;

      if(cmdline.isset("outfile"))
      {
        auto file_name = cmdline.get_value("outfile");
#ifdef _WIN32
        std::ofstream out(widen(file_name));
#else
        std::ofstream out(file_name);
#endif
        if(!out)
        {
          std::cerr << "failed to open " << file_name << '\n';
          return CPROVER_EXIT_PARSE_ERROR;
        }

        if(variable_encoding)
          ::variable_encoding(goto_model, format, out);
        else
          state_encoding(goto_model, format, perform_inlining, out);

        std::cout << "formula written to " << file_name << '\n';
      }
      else
      {
        if(variable_encoding)
          ::variable_encoding(goto_model, format, std::cout);
        else
          state_encoding(goto_model, format, perform_inlining, std::cout);
      }

      if(!cmdline.isset("solve"))
        return CPROVER_EXIT_SUCCESS;
    }

    std::size_t loop_limit;

    if(cmdline.isset("unwind"))
    {
      loop_limit = std::stoull(cmdline.get_value("unwind"));
    }
    else
      loop_limit = 1;

    // solve
    auto result =
      state_encoding_solver(goto_model, perform_inlining, loop_limit);

    switch(result)
    {
    case solver_resultt::ALL_PASS:
      return CPROVER_EXIT_SUCCESS;

    case solver_resultt::SOME_FAIL:
      return CPROVER_EXIT_VERIFICATION_UNSAFE;

    case solver_resultt::ERROR:
      return CPROVER_EXIT_INTERNAL_ERROR;
    }
  }
  catch(const cprover_exception_baset &e)
  {
    std::cerr << "error: " << e.what() << '\n';
    return CPROVER_EXIT_EXCEPTION;
  }

  UNREACHABLE; // to silence a gcc warning
}

/// display command line help
void cprover_parse_optionst::help()
{
  std::cout << '\n';

  std::cout
    << u8"* * CPROVER " << CBMC_VERSION << " (" << (sizeof(void *) * 8)
    << "-bit)"
    << " * *\n"
    << "* *                       Copyright 2022                       * *\n";

  // clang-format off
  std::cout << help_formatter(
    "\n"
    "Usage:                     \tPurpose:\n"
    "\n"
    " {bcprover} [{y-?}] [{y-h}] [{y--help}] \t show this help\n"
    " {bcprover} {usource-file.c}    \t convert a given C program\n"
    "\n"
    "Other options:\n"
    " {y--inline} \t perform function call inlining before\n"
    " \t generating the formula\n"
    " {y--outfile} {ufile-name} \t set output file for formula\n"
    " {y--smt2} \t output formula in SMT-LIB2 format\n"
    " {y--text} \t output formula in text format\n"
    "\n");
}
