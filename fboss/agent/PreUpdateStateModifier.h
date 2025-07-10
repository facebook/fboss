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

#include <boost/core/noncopyable.hpp>

#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

class PreUpdateStateModifier : public boost::noncopyable {
 public:
  virtual ~PreUpdateStateModifier() {}
  virtual std::vector<StateDelta> modifyState(
      const std::vector<StateDelta>& deltas) = 0;
  virtual void updateFailed(const std::shared_ptr<SwitchState>& state) = 0;
  virtual void updateDone() = 0;
};

} // namespace facebook::fboss
