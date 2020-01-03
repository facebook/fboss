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

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

class StateObserver : public boost::noncopyable {
 public:
  virtual ~StateObserver() {}
  virtual void stateUpdated(const StateDelta& delta) = 0;
};

class AutoRegisterStateObserver : public StateObserver {
 public:
  AutoRegisterStateObserver(SwSwitch* sw, const std::string& name) : sw_(sw) {
    sw_->registerStateObserver(this, name);
  }
  ~AutoRegisterStateObserver() override {
    sw_->unregisterStateObserver(this);
  }

  // This empty implementation should be overridden by subclasses, but it is
  // needed during destruction in the case that the derived class has been
  // destroyed, but the unregisterStateObserver call is blocked. If someone
  // has a pointer to the observer, they could call a pure virtual method
  // during that time if this didn't exist.
  void stateUpdated(const StateDelta& /*delta*/) override {}

 private:
  SwSwitch* sw_{nullptr};
};

} // namespace facebook::fboss
