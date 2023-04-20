// Author: Fotis Koutoulakis for Diffblue Ltd.

#ifndef CPROVER_CORE_BMC_PARSE_OPTIONS_H
#define CPROVER_CORE_BMC_PARSE_OPTIONS_H

#include <memory>
#include <string>
#include <vector>

api_optionst parse_command_line_options(const std::vector<std::string> &);

void help();

#endif // CPROVER_CORE_BMC_PARSE_OPTIONS_H
