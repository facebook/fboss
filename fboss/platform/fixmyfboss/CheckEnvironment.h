/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <memory>
#include <optional>
#include <string>

#include "fboss/platform/platform_checks/Host.h"
#include "fboss/platform/platform_checks/RemoteHost.h"

namespace facebook::fboss::platform::fixmyfboss {

/**
 * The switch being diagnosed: either the machine fixmyfboss runs on, or a
 * remote switch reached over SSH.
 */
struct CheckEnvironment {
  // Platform of the x86, used to select configs.
  std::string platformName;
  std::shared_ptr<const platform_checks::Host> x86;
};

struct ConnectOptions {
  // Unset means the machine fixmyfboss runs on.
  std::optional<std::string> hostname;
  platform_checks::RemoteHost::Transport transport{
      platform_checks::RemoteHost::Transport::SSH};
};

// Connects to the switch and resolves its platform. Throws std::runtime_error
// with a user-facing message if the switch is unreachable or its platform is
// unknown.
CheckEnvironment createEnvironment(const ConnectOptions& options);

} // namespace facebook::fboss::platform::fixmyfboss
