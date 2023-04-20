// Author: Enrico Steffinlongo for Diffblue Ltd.

#include <util/exception_utils.h>

#include <libcprover-cpp/api.h>
#include <libcprover-cpp/api_options.h>

#include "corebmc_parse_options.h"

#include <iostream>
#include <vector>

void print_messages_to_stdout(
  const api_messaget &message,
  api_call_back_contextt)
{
  std::cout << api_message_get_string(message) << std::endl;
}

int main(int argc, char *argv[])
{
  // Parsing of arguments works on vector, so we need to do some type-jitsu
  std::vector<std::string> arguments(argv + 1, argv + argc);
  auto api_options = parse_command_line_options(arguments);

  // With options set, we can now initiate the API and perform analysis.
  api_sessiont api(api_options);
  std::cout << "CoreBMC version ";
  std::cout << *api.get_api_version() << std::endl;

  // The API works with a callback for querying state - we for now just print
  // collected messages from the message buffer to stdout.
  api.set_message_callback(print_messages_to_stdout, nullptr);

  // Finally, we read the goto-binary the user supplied...
  api.read_goto_binary(arguments[0]);

  // ... and run analysis on it.
  auto result = api.verify_model();

  return 0;
}
