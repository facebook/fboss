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

#include <folly/experimental/TestUtil.h>

#include "fboss/agent/platforms/test_platforms/BcmTestPlatform.h"

namespace facebook {
namespace fboss {
class FakeBcmTestPlatform : public BcmTestPlatform {
 public:
  FakeBcmTestPlatform();
  ~FakeBcmTestPlatform() override {}

  bool isCosSupported() const override {
    return true;
  }

  bool v6MirrorTunnelSupported() const override {
    return true;
  }

  bool sflowSamplingSupported() const override {
    return true;
  }

  bool mirrorPktTruncationSupported() const override {
    return true;
  }

  std::vector<FlexPortMode> getSupportedFlexPortModes() const override {
    return {FlexPortMode::ONEX100G,
            FlexPortMode::TWOX50G,
            FlexPortMode::ONEX40G,
            FlexPortMode::FOURX25G,
            FlexPortMode::FOURX10G};
  }

  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;
  bool hasLinkScanCapability() const override {
    return false;
  }
  bool isBcmShellSupported() const override {
    return false;
  }
  const PortQueue& getDefaultPortQueueSettings(
      cfg::StreamType streamType) const override;
  const PortQueue& getDefaultControlPlaneQueueSettings(
      cfg::StreamType streamType) const override;

  uint32_t getMMUBufferBytes() const override {
    return 16 * 1024 * 1024;
  }
  uint32_t getMMUCellBytes() const override {
    return 208;
  }

  bool useQueueGportForCos() const override {
    return true;
  }

  uint32_t maxLabelStackDepth() const override {
    // for no reason :)
    return 9;
  }

  bool isMultiPathLabelSwitchActionSupported() const override {
    return true;
  }

  HwAsic* getAsic() const override {
    /* TODO: implement this */
    return nullptr;
  }

 private:
  // Forbidden copy constructor and assignment operator
  FakeBcmTestPlatform(FakeBcmTestPlatform const&) = delete;
  FakeBcmTestPlatform& operator=(FakeBcmTestPlatform const&) = delete;

  std::unique_ptr<BcmTestPort> createTestPort(PortID id) const override;

  folly::test::TemporaryDirectory tmpDir_;
};
} // namespace fboss
} // namespace facebook
