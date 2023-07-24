// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>

namespace facebook::fboss::platform::helpers {

/*
 * exec command, return contents of stdout and fill in exit status
 */
std::string execCommandUnchecked(const std::string& cmd, int& exitStatus);

} // namespace facebook::fboss::platform::helpers
