// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <array>
#include <memory>
#include <mutex>

#include <folly/Singleton.h>

#include "fboss/agent/types.h"
#include "fboss/lib/fpga/FbDomFpga.h"

namespace facebook {
namespace fboss {

class Wedge400FpgaImpl {
 public:
  uint32_t read(TransceiverID tcvr, uint32_t offset) const;

  void write(TransceiverID tcvr, uint32_t offset, uint32_t value);

  void
  setLedStatus(TransceiverID tcvr, uint8_t channel, FbDomFpga::LedColor color);

  FbDomFpga::LedColor getLedStatus(TransceiverID tcvr, uint8_t channel) const;

  void triggerQsfpHardReset(TransceiverID tcvr);

  void clearAllTransceiverReset();

  Wedge400FpgaImpl(
      std::unique_ptr<FpgaDevice> leftFpga,
      std::unique_ptr<FpgaDevice> rightFpga);

  FbDomFpga* getLeftFpga() const {
    return leftFpga_.get();
  }
  FbDomFpga* getRightFpga() const {
    return rightFpga_.get();
  }
  FbDomFpga* getFpga(TransceiverID tcvr) const;

 private:
  std::unique_ptr<FpgaDevice> leftDevice_;
  std::unique_ptr<FpgaDevice> rightDevice_;
  std::unique_ptr<FbDomFpga> leftFpga_;
  std::unique_ptr<FbDomFpga> rightFpga_;

  std::array<std::mutex, 16> locks_;

  inline uint32_t getFpgaPortIndex(TransceiverID tcvr) const;

  inline uint32_t getPortLedAddress(TransceiverID tcvr) const;

  inline uint32_t tcvrLedGroup(TransceiverID tcvr) const;
};

class Wedge400Fpga {
 public:
  static std::shared_ptr<Wedge400Fpga> getInstance();

  uint32_t read(TransceiverID tcvr, uint32_t offset) const {
    return impl->read(tcvr, offset);
  }

  void write(TransceiverID tcvr, uint32_t offset, uint32_t value) {
    impl->write(tcvr, offset, value);
  }

  void
  setLedStatus(TransceiverID tcvr, uint8_t channel, FbDomFpga::LedColor color) {
    impl->setLedStatus(tcvr, channel, color);
  }

  FbDomFpga::LedColor getLedStatus(TransceiverID tcvr, uint8_t channel) const {
    return impl->getLedStatus(tcvr, channel);
  }

  void triggerQsfpHardReset(TransceiverID tcvr) {
    impl->triggerQsfpHardReset(tcvr);
  }

  void clearAllTransceiverReset() {
    impl->clearAllTransceiverReset();
  }

  // TODO: These functions are temporary for Wedge400I2C due to the fpga
  // refactor. We should remove these and have everone use ::getFpga
  FbDomFpga* getLeftFpga() const {
    return impl->getLeftFpga();
  }
  FbDomFpga* getRightFpga() const {
    return impl->getRightFpga();
  }

  FbDomFpga* getFpga(TransceiverID tcvr) const {
    return impl->getFpga(tcvr);
  }

 private:
  std::unique_ptr<Wedge400FpgaImpl> impl;

  template <typename, typename, typename>
  friend class folly::Singleton;
  Wedge400Fpga();
};

} // namespace fboss
} // namespace facebook
