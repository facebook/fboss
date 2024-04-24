// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gflags/gflags.h>

DECLARE_bool(help);
DECLARE_string(helpmatch);

namespace facebook::fboss::platform::helpers {

// `helpmatch` controls the help message that is printed when running with
// --help. Refer to FLAGS_helpmatch in gflags.cc for more details.
void initCli(int* argc, char*** argv, const std::string& helpmatch);

} // namespace facebook::fboss::platform::helpers
