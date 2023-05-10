// Author: Fotis Koutoulakis for Diffblue Ltd.

#ifndef CPROVER_CORE_BMC_PARSE_OPTIONS_H
#define CPROVER_CORE_BMC_PARSE_OPTIONS_H

#include <util/parse_options.h>
#include <util/version.h>

#include <libcprover-cpp/api_options.h>

#include <memory>
#include <string>
#include <vector>

api_optionst parse_command_line_options(const std::vector<std::string> &);

void help();

// clang-format off
#define CORE_CBMC_ONLY_OPTIONS \
  "(version)"
// clang-format on

class core_cbmc_only_parse_optionst : public parse_options_baset
{
public:
  core_cbmc_only_parse_optionst(int argc, const char **argv);

  virtual void help();

  virtual int doit();
};

#endif // CPROVER_CORE_BMC_PARSE_OPTIONS_H
