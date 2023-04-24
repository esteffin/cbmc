// Author: Fotis Koutoulakis for Diffblue Ltd.

#ifndef CPROVER_CORE_BMC_PARSE_OPTIONS_H
#define CPROVER_CORE_BMC_PARSE_OPTIONS_H

#include <util/parse_options.h>
#include <util/version.h>

#include <libcprover-cpp/api_options.h>

#include <memory>
#include <string>
#include <vector>

#define CORE_CBMC_ONLY_OPTIONS                                                           \
  "(unwinding-assertions)"                                                     \
  "(no-unwinding-assertions)"                                                  \
  "(unwind):"                                                                  \
  "(unwindset):"                                                               \
  "(unwind-min):"                                                              \
  "(unwind-max):"                                                              \
  "(version)"

class core_cbmc_only_parse_optionst : public parse_options_baset
{
public:
  core_cbmc_only_parse_optionst(int argc, const char **argv);

  virtual int doit();
};

#endif // CPROVER_CORE_BMC_PARSE_OPTIONS_H
