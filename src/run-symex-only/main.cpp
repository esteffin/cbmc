// Author: Enrico Steffinlongo for Diffblue Ltd.

#include <util/exception_utils.h>

#include <libcprover-cpp/api.h>
#include <libcprover-cpp/api_options.h>

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
  try
  {
    std::cout << "CoreBMC version ";

    std::vector<std::string> arguments(argv + 1, argv + argc);

    // Create API options object, to pass to initialiser of API object.
    auto api_options = api_optionst::create()
                         .simplify(false)
                         .validate_goto_model(true)
                         .drop_unused_functions(true);

    api_sessiont api(api_options);
    std::cout << *api.get_api_version() << std::endl;

    api.set_message_callback(print_messages_to_stdout, nullptr);
    // TODO: Guard properly for argv size
    api.read_goto_binary(arguments[0]);

    auto result = api.verify_model();

    return 0;
  }
  catch(const invalid_command_line_argument_exceptiont &e)
  {
    std::cout << e.what() << std::endl;
  }
}
