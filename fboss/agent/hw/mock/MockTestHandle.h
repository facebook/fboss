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
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class MockTestHandle : public HwTestHandle {
 public:
  MockTestHandle(std::unique_ptr<SwSwitch> sw, MockPlatform* platform)
      : HwTestHandle(std::move(sw), platform), platform_(platform) {}
  ~MockTestHandle() {}

  void rxPacket(
      std::unique_ptr<folly::IOBuf> buf,
      PortID srcPort,
      std::optional<VlanID> srcVlan) override;
  void forcePortDown(PortID port) override;
  void forcePortUp(PortID port) override;
  void forcePortFlap(PortID port) override;

 private:
  // Forbidden copy constructor and assignment operator
  MockTestHandle(MockTestHandle const&) = delete;
  MockTestHandle& operator=(MockTestHandle const&) = delete;

  MockPlatform* platform_{nullptr};
};

} // namespace facebook::fboss
