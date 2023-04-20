// Author: Fotis Koutoulakis for Diffblue Ltd.

#include "corebmc_parse_options.h"

#include <util/message.h>

#include <goto-programs/read_goto_binary.h>

#include <libcprover-cpp/api_options.h>

#include <iostream>
#include <map>

api_optionst
parse_command_line_options(const std::vector<std::string> &arguments)
{
  // Before anything else, make sure that the first argument is the input goto-binary.
  null_message_handlert null_handler{};
  if(!::is_goto_binary(arguments[0], null_handler))
  {
    std::cerr << "Error: first argument should be goto-binary filename"
              << std::endl;
    help();
    abort();
  }

  auto api_options = api_optionst::create();

  auto option_is_present = [&arguments](const std::string &argument)
  {
    return std::find(arguments.begin(), arguments.end(), argument) !=
           arguments.end();
  };

  if(option_is_present("--validate-goto-model"))
  {
    api_options.validate_goto_model(true);
  }

  return api_options;
}

void help()
{
  std::cerr << "core-bmc <input-file> --option1 ... --option_n" << std::endl;
}
