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

#include "fboss/lib/fpga/FbFpgaI2c.h"
#include "fboss/lib/i2c/gen-cpp2/i2c_controller_stats_types.h"
#include "fboss/lib/usb/PCA9548MuxedBus.h"

namespace facebook::fboss {
class MinipackI2cError : public I2cError {
 public:
  explicit MinipackI2cError(const std::string& what) : I2cError(what) {}
};

class Minipack16QI2CBus : public TransceiverI2CApi {
 public:
  Minipack16QI2CBus();
  ~Minipack16QI2CBus() override;
  void open() override {}
  void close() override {}

  void moduleRead(
      unsigned int module,
      uint8_t i2cAddress,
      int offset,
      int len,
      uint8_t* buf) override;
  void moduleWrite(
      unsigned int module,
      uint8_t i2cAddress,
      int offset,
      int len,
      const uint8_t* buf) override;

  bool isPresent(unsigned int module) override;
  void scanPresence(std::map<int32_t, ModulePresence>& presences) override;
  void ensureOutOfReset(unsigned int module) override;
  void verifyBus(bool /* autoReset */) override {}

  /* Consolidate the i2c transaction stats from all the pims using their
   * corresponding i2c controller. In case of Minipack16q there are 8 pims
   * and there are four FbFpgaI2cController corresponding to each pim. This
   * function consolidates the counters from all constollers and return the
   * vector of the i2c stats
   */
  std::vector<std::reference_wrapper<const I2cControllerStats>>
  getI2cControllerStats() override;

  folly::EventBase* getEventBase(unsigned int module) override;

 private:
  FbFpgaI2cController* getI2cController(uint8_t pim, uint8_t idx) const;

  static constexpr uint8_t MODULES_PER_PIM = 16;

  std::array<std::array<std::unique_ptr<FbFpgaI2cController>, 4>, 8>
      i2cControllers_;
};

} // namespace facebook::fboss
