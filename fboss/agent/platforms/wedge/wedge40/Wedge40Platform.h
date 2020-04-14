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

#include "fboss/agent/hw/switch_asics/Trident2Asic.h"
#include "fboss/agent/platforms/wedge/WedgePlatform.h"

namespace facebook::fboss {

class WedgePortMapping;
class PlatformProductInfo;

class Wedge40Platform : public WedgePlatform {
 public:
  explicit Wedge40Platform(std::unique_ptr<PlatformProductInfo> productInfo);

  std::unique_ptr<WedgePortMapping> createPortMapping() override;

  folly::ByteRange defaultLed0Code() override;
  folly::ByteRange defaultLed1Code() override;

  bool v6MirrorTunnelSupported() const override {
    return false;
  }
  bool sflowSamplingSupported() const override {
    return false;
  }
  bool mirrorPktTruncationSupported() const override {
    return false;
  }
  uint32_t getMMUBufferBytes() const override {
    return 16 * 1024 * 1024;
  }
  uint32_t getMMUCellBytes() const override {
    return 208;
  }
  const PortQueue& getDefaultPortQueueSettings(
      cfg::StreamType streamType) const override;
  const PortQueue& getDefaultControlPlaneQueueSettings(
      cfg::StreamType streamType) const override;

  bool useQueueGportForCos() const override {
    return false;
  }

  HwAsic* getAsic() const override {
    return asic_.get();
  }

 private:
  Wedge40Platform(Wedge40Platform const&) = delete;
  Wedge40Platform& operator=(Wedge40Platform const&) = delete;
  std::unique_ptr<Trident2Asic> asic_;
};

} // namespace facebook::fboss
