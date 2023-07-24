// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>

namespace facebook::fboss::platform::helpers {

/*
 * exec command, return contents of stdout and fill in exit status
 */
std::string execCommandUnchecked(const std::string& cmd, int& exitStatus);
/*
 * exec command, return contents of stdout. Throw on command failing (exit
 * status != 0)
 */
std::string execCommand(const std::string& cmd);

} // namespace facebook::fboss::platform::helpers
