// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/bsp/BspSystemContainer.h"
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

namespace facebook {
namespace fboss {

class BspIOBus : public TransceiverI2CApi {
 public:
  explicit BspIOBus(const BspSystemContainer* systemContainer)
      : systemContainer_(systemContainer) {}
  ~BspIOBus() override {}

  void open() override {
    initTransceivers();
  }

  void close() override {}
  void verifyBus(bool /* autoReset */) override {}

  void moduleRead(
      unsigned int module,
      const TransceiverAccessParameter& param,
      uint8_t* buf) override;
  void moduleWrite(
      unsigned int module,
      const TransceiverAccessParameter& param,
      const uint8_t* buf) override;
  bool isPresent(unsigned int module) override;
  void scanPresence(std::map<int32_t, ModulePresence>& presences) override;
  std::vector<I2cControllerStats> getI2cControllerStats() override;
  folly::EventBase* getEventBase(unsigned int module) override {
    return systemContainer_->getPimContainerFromTcvrID(module)->getIOEventBase(
        module);
  }
  // Forbidden copy constructor and assignment operator
  BspIOBus(BspIOBus const&) = delete;
  BspIOBus& operator=(BspIOBus const&) = delete;

 private:
  void initTransceivers();
  const BspSystemContainer* systemContainer_;
};

} // namespace fboss
} // namespace facebook
