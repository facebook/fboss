/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <CLI/CLI.hpp>
#pragma once

namespace facebook::fboss::utils {

CLI::App* getSubcommandIf(const CLI::App& cmd, const std::string& subcommand);

} // namespace facebook::fboss::utils
