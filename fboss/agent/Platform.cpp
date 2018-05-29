/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Platform.h"

#include <string>
#include "fboss/agent/state/SwitchState.h"

DEFINE_string(crash_switch_state_file, "crash_switch_state",
              "File for dumping SwitchState state on crash");

DEFINE_string(crash_hw_state_file, "crash_hw_state",
              "File for dumping HW state on crash");

namespace facebook { namespace fboss {

std::string Platform::getCrashHwStateFile() const {
  return getCrashInfoDir() + "/" + FLAGS_crash_hw_state_file;
}

std::string Platform::getCrashSwitchStateFile() const {
  return getCrashInfoDir() + "/" + FLAGS_crash_switch_state_file;
}

}} //facebook::fboss
