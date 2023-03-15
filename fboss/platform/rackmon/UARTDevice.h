// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once

#include <mutex>
#include <vector>
#include "Device.h"

namespace rackmon {

enum class Parity {
  EVEN,
  ODD,
  NONE,
};

class UARTDevice : public Device {
  int baudrate_ = -1;
  Parity parity_ = Parity::EVEN;

 protected:
  virtual void setAttribute(bool readEnable, int baudrate, Parity parity);

  void readEnable() {
    setAttribute(true, baudrate_, parity_);
  }
  void readDisable() {
    setAttribute(false, baudrate_, parity_);
  }

 public:
  UARTDevice(const std::string& device, int baudrate)
      : Device(device), baudrate_(baudrate) {}

  int getBaudrate() const {
    return baudrate_;
  }
  void setBaudrate(int baudrate) {
    if (baudrate == baudrate_) {
      return;
    }
    baudrate_ = baudrate;
    setAttribute(true, baudrate, parity_);
  }

  Parity getParity() const {
    return parity_;
  }
  void setParity(Parity parity) {
    if (parity == parity_) {
      return;
    }
    parity_ = parity;
    setAttribute(true, baudrate_, parity);
  }

  void open() override;
  void write(const uint8_t* buf, size_t len) override;
};

class AspeedRS485Device : public UARTDevice {
 protected:
  void waitWrite() override;

 public:
  AspeedRS485Device(const std::string& device, int baudrate)
      : UARTDevice(device, baudrate) {}
  void open() override;
  void write(const uint8_t* buf, size_t len) override;
};

class LocalEchoUARTDevice : public UARTDevice {
 public:
  LocalEchoUARTDevice(const std::string& device, int baudrate)
      : UARTDevice(device, baudrate) {}
  void write(const uint8_t* buf, size_t len) override;
};

} // namespace rackmon
