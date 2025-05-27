// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/commands/show/platformshowtech/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss::show::platformshowtech::utils {

int run_cmd(std::string cmd, std::string &output);
std::string run_cmd_no_check(std::string cmd);
std::string run_cmd_with_limit(std::string cmd, int max_lines = 5000);
void print_fboss2_show_cmd(std::string cmd);
void strip(std::string &str);
int get_max_i2c_bus();

} // namespace facebook::fboss::show::platformshowtech::utils
