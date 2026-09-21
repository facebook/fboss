// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/FbFpgaI2c.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

namespace facebook::fboss {
class Wedge400I2CBus : public TransceiverI2CApi {
 public:
  Wedge400I2CBus();
  ~Wedge400I2CBus() override;

  void open() override {}
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
  void ensureOutOfReset(unsigned int module) override;

  /* Get the i2c transaction stats from I2C controllers in Wedge400. In case
   * of wedge400 there are six FbFpgaI2cController for left side modules and
   * six for right side modules. This function put the I2c controller stats
   * from each controller in a vector and returns to the caller
   */
  std::vector<I2cControllerStats> getI2cControllerStats() override;

  folly::EventBase* getEventBase(unsigned int module) override;

 private:
  FbFpgaI2cController* getI2cController(unsigned int module);

  static constexpr uint8_t I2C_CTRL_PER_PIM = 6;

  FbDomFpga* leftFpga_;
  FbDomFpga* rightFpga_;

  std::array<std::unique_ptr<FbFpgaI2cController>, I2C_CTRL_PER_PIM>
      leftI2cControllers_;
  std::array<std::unique_ptr<FbFpgaI2cController>, I2C_CTRL_PER_PIM>
      rightI2cControllers_;
};

} // namespace facebook::fboss
