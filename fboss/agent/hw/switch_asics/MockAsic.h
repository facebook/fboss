// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss {

class MockAsic : public HwAsic {
 public:
  bool isSupported(Feature feature) const override {
    return feature != Feature::HSDK; // Mock supports all features but HSDK
  }
  AsicType getAsicType() const override {
    return AsicType::ASIC_TYPE_MOCK;
  }
  std::string getVendor() const override {
    return "mock";
  }
  cfg::PortSpeed getMaxPortSpeed() const override {
    return cfg::PortSpeed::HUNDREDG;
  }
  std::set<cfg::StreamType> getQueueStreamTypes(bool cpu) const override {
    if (cpu) {
      return {cfg::StreamType::MULTICAST};
    } else {
      return {cfg::StreamType::UNICAST};
    }
  }
  int getDefaultNumPortQueues(cfg::StreamType /* streamType */) const override {
    return 10;
  }
  uint32_t getMaxLabelStackDepth() const override {
    // Copying TH3's max label stack depth
    return 9;
  }
  bool needsObjectKeyCache() const override {
    return false;
  }
  uint64_t getMMUSizeBytes() const override {
    // Fake MMU size
    return 64 * 1024 * 1024;
  }
};

} // namespace facebook::fboss
