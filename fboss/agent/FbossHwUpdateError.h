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

#include <folly/Conv.h>
#include "fboss/agent/FbossError.h"

#include <memory>

namespace facebook::fboss {

class SwitchState;

class FbossHwUpdateError : public FbossError {
 public:
  template <typename... Args>
  FbossHwUpdateError(
      const std::shared_ptr<SwitchState>& _desiredState,
      const std::shared_ptr<SwitchState>& _appliedState,
      Args&&... args)
      : FbossError(std::forward<Args>(args)...),
        desiredState(_desiredState),
        appliedState(_appliedState) {}

  std::shared_ptr<SwitchState> desiredState;
  std::shared_ptr<SwitchState> appliedState;
};

} // namespace facebook::fboss
