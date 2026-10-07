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
#include <string>

#include "fboss/platform/platform_checks/Host.h"

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

} // namespace facebook::fboss::platform::fixmyfboss
