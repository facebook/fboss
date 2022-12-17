// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/MinipackBaseSystemContainer.h"
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

namespace facebook::fboss {

class MinipackI2cError : public I2cError {
 public:
  explicit MinipackI2cError(const std::string& what) : I2cError(what) {}
};

class MinipackBaseI2cBus : public TransceiverI2CApi {
 public:
  MinipackBaseI2cBus();
  virtual ~MinipackBaseI2cBus() override {}
  void open() override {}
  void close() override {}

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
  void verifyBus(bool /* autoReset */) override {}

  void ensureOutOfReset(unsigned int module) override;
  folly::EventBase* getEventBase(unsigned int module) override;

 protected:
  uint32_t portsPerPim_ = 16;
  virtual uint8_t getPim(int module);
  virtual uint8_t getQsfpPimPort(int module);
  virtual int getModule(uint8_t pim, uint8_t port);
  std::shared_ptr<MinipackBaseSystemContainer> systemContainer_;
  FbFpgaI2cController* getI2cController(uint8_t pim, uint8_t idx) const;
};

} // namespace facebook::fboss
