// Author: Enrico Steffinlongo for Diffblue Ltd.

#include "goto_bmc_parse_options.h"

#include <iostream>
#include <vector>

int main(int argc, const char **argv)
{
  return core_cbmc_only_parse_optionst{argc, argv}.main();
}
