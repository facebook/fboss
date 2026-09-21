/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/logging/xlog.h>
#include <memory>
#include <optional>
#include <string>

#include "fboss/agent/SysError.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"

namespace facebook::fboss {

class SwitchState;

/**
 * Utility class for common warm boot file operations.
 * This class provides functionality shared between HwSwitchWarmBootHelper
 * and SwSwitchWarmBootHelper.
 */
class WarmBootFileUtils {
 public:
  /**
   * Creates a warm boot flag file at the specified path.
   * This flag indicates that warm boot is possible on the next startup.
   */
  static void setCanWarmBoot(const std::string& flagPath);

  /**
   * Reads a Thrift state from a binary file.
   * Throws FbossError if reading fails.
   */
  static state::SwitchState getWarmBootThriftState(const std::string& filePath);

  /**
   * Stores a Thrift state to a binary file.
   * Logs a FATAL error if writing fails.
   */
  static void storeWarmBootThriftState(
      const std::string& filePath,
      const state::SwitchState& switchThriftState);

  /**
   * Reconstructs a SwitchState from a Thrift state.
   * Returns nullptr if warmBootState is not present.
   */
  static std::shared_ptr<SwitchState> reconstructWarmBootThriftState(
      const std::optional<state::SwitchState>& warmBootState);

  /**
   * Reads a WarmbootState from a binary file.
   * Throws FbossError if reading fails.
   */
  static state::WarmbootState getWarmBootState(const std::string& filePath);

  /**
   * Stores a WarmbootState to a binary file.
   * Logs a FATAL error if writing fails.
   */
  static void storeWarmBootState(
      const std::string& filePath,
      const state::WarmbootState& switchStateThrift);
};

} // namespace facebook::fboss
