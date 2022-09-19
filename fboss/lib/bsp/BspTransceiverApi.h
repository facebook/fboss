// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspSystemContainer.h"
#include "fboss/lib/usb/TransceiverPlatformApi.h"

namespace facebook {
namespace fboss {

class BspTransceiverApi : public TransceiverPlatformApi {
 public:
  explicit BspTransceiverApi(const BspSystemContainer* systemContainer);
  ~BspTransceiverApi() override {}
  void triggerQsfpHardReset(unsigned int module) override;
  void clearAllTransceiverReset() override;

  // Forbidden copy constructor and assignment operator
  BspTransceiverApi(BspTransceiverApi const&) = delete;
  BspTransceiverApi& operator=(BspTransceiverApi const&) = delete;

 private:
  const BspSystemContainer* systemContainer_;
};

} // namespace fboss
} // namespace facebook
