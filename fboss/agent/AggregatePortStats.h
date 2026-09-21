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

#include <fb303/ThreadCachedServiceData.h>
#include "fboss/agent/types.h"

#include <string>

namespace facebook::fboss {

class AggregatePort;
class SwSwitch;

class AggregatePortStats {
 public:
  AggregatePortStats(
      AggregatePortID aggregatePortID,
      std::string aggregatePortName);

  void flapped();
  void aggregatePortNameChanged(const std::string& name);

 private:
  std::string constructCounterName(
      const std::string& aggregatePortName,
      const std::string& counter) const;

  // Forbidden copy constructor and assignment operator
  AggregatePortStats(const AggregatePortStats&) = delete;
  AggregatePortStats& operator=(const AggregatePortStats&) = delete;

  AggregatePortID aggregatePortID_;
  std::string aggregatePortName_;

  fb303::ThreadCachedServiceData::TLTimeseries flaps_;
};

} // namespace facebook::fboss
