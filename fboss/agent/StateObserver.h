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

class StateObserver : public boost::noncopyable {
 public:
  virtual ~StateObserver() {}
  virtual void stateUpdated(const StateDelta& delta) = 0;
};

} // namespace facebook::fboss
