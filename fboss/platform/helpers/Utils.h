// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>

namespace facebook::fboss::platform::helpers {

// Executes command and returns exit status and standard output.
std::pair<int, std::string> execCommand(const std::string& cmd);

} // namespace facebook::fboss::platform::helpers
