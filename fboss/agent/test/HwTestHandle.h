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

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class HwTestHandle {
 public:
  explicit HwTestHandle(std::unique_ptr<SwSwitch> sw) : sw_(std::move(sw)) {}
  virtual ~HwTestHandle() = default;

  SwSwitch* getSw() const {
    return sw_.get();
  }

  virtual void prepareForTesting() {}

  // Useful helpers for testing low level events
  virtual void rxPacket(
      std::unique_ptr<folly::IOBuf> buf,
      const PortID srcPort,
      const std::optional<VlanID> srcVlan) = 0;
  virtual void forcePortDown(const PortID port) = 0;
  virtual void forcePortUp(const PortID port) = 0;
  virtual void forcePortFlap(const PortID port) = 0;

 private:
  std::unique_ptr<SwSwitch> sw_{nullptr};
};

} // namespace facebook::fboss
