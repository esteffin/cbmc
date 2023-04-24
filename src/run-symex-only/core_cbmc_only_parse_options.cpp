// Author: Fotis Koutoulakis for Diffblue Ltd.

#include "core_cbmc_only_parse_options.h"

#include <util/exit_codes.h>
#include <util/message.h>

#include <goto-programs/read_goto_binary.h>

#include <libcprover-cpp/api_options.h>

#include "api.h"

#include <iostream>

core_cbmc_only_parse_optionst::core_cbmc_only_parse_optionst(
  int argc,
  const char **argv)
  : parse_options_baset(
      CORE_CBMC_ONLY_OPTIONS,
      argc,
      argv,
      std::string("CORE_CBMC_ONLY") + CBMC_VERSION)
{
}

void print_messages_to_stdout(
  const api_messaget &message,
  api_call_back_contextt)
{
  std::cout << api_message_get_string(message) << std::endl;
}

int core_cbmc_only_parse_optionst::doit()
{
  auto api_options = api_optionst::create();

  if(cmdline.isset("version"))
  {
    std::cout << CBMC_VERSION << '\n';
    return CPROVER_EXIT_SUCCESS;
  }

  if(
    cmdline.args.size() != 1 ||
    !is_goto_binary(cmdline.args[0], ui_message_handler))
  {
    log.error() << "Please give exactly one binary file" << messaget::eom;
    return CPROVER_EXIT_INCORRECT_TASK;
  }

  std::string filename = cmdline.args[0];

  if(cmdline.isset("validate-goto-model"))
  {
    api_options.validate_goto_model(true);
  }

  if (cmdline.isset("unwinding-assertions") && cmdline.isset("no-unwinding-assertions"))
  {
    log.error() << "Option --unwinding-assertions conflicts with --no-unwinding-assertions" << messaget::eom;
    return CPROVER_EXIT_USAGE_ERROR;
  }

  if(cmdline.isset("unwinding-assertions"))
  {
//    api_options.unwinding_assertion = true;
    // api_options.paths_symex_explore_all = true;
  }
  if(cmdline.isset("unwind"))
  {
//    api_options.unwind = cmdline.get_value("unwind");
    if(!cmdline.isset("unwinding-assertions"))
    {
      log.warning() << "**** WARNING: Use --unwinding-assertions to obtain "
                       "sound verification results"
                    << messaget::eom;
    }
  }
  if(cmdline.isset("unwindset"))
  {
//    api_options.unwindset = cmdline.get_comma_separated_values("unwindset");
    if(!cmdline.isset("unwinding-assertions"))
    {
      log.warning() << "**** WARNING: Use --unwinding-assertions to obtain "
                       "sound verification results"
                    << messaget::eom;
    }
  }
  if(cmdline.isset("unwind-min"))
  {
    api_options.unwind_min(std::stoi(cmdline.get_value("unwind-min")));
  }
  if(cmdline.isset("unwind-max"))
  {
//    api_options.unwind_max = cmdline.get_value("unwind-max");
  }

  // With options set, we can now initiate the API and perform analysis.
  api_sessiont api{api_options};
  std::cout << "CoreBMC version ";
  std::cout << *api.get_api_version() << std::endl;

  // The API works with a callback for querying state - we for now just print
  // collected messages from the message buffer to stdout.
  api.set_message_callback(print_messages_to_stdout, nullptr);

  // Finally, we read the goto-binary the user supplied...
  api.read_goto_binary(filename);

  // ... and run analysis on it.
  auto result = api.verify_model();

  return CPROVER_EXIT_SUCCESS;
}
