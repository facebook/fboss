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

#include <folly/io/IOBuf.h>

#include "fboss/agent/Platform.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class Platform;

class HwTestHandle {
 public:
  HwTestHandle(
      std::unique_ptr<SwSwitch> sw,
      std::vector<std::unique_ptr<Platform>> platforms)
      : platforms_(std::move(platforms)), sw_(std::move(sw)) {}

  virtual ~HwTestHandle() = default;

  SwSwitch* getSw() const {
    return sw_.get();
  }

  Platform* getPlatform() const {
    return getPlatform(0);
  }

  Platform* getPlatform(int switchIndex) const {
    return platforms_.at(switchIndex).get();
  }

  HwSwitch* getHwSwitch() const {
    return getHwSwitch(0);
  }

  HwSwitch* getHwSwitch(int switchIndex) const {
    return getPlatform(switchIndex)->getHwSwitch();
  }

  bool multiSwitch() const {
    return platforms_.size() > 1;
  }

  virtual void prepareForTesting() {}

  // Useful helpers for testing low level events
  virtual void rxPacket(
      std::unique_ptr<folly::IOBuf> buf,
      const PortDescriptor& srcPort,
      const std::optional<VlanID> srcVlan) = 0;
  virtual void forcePortDown(const PortID port) = 0;
  virtual void forcePortUp(const PortID port) = 0;
  virtual void forcePortFlap(const PortID port) = 0;

 private:
  std::vector<std::unique_ptr<Platform>> platforms_;
  std::unique_ptr<SwSwitch> sw_{nullptr};
};

} // namespace facebook::fboss
