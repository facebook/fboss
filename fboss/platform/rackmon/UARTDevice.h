// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once

#include <mutex>
#include <vector>
#include "Device.h"

namespace rackmon {

class UARTDevice : public Device {
  int baudrate_ = -1;

 protected:
  void waitWrite() override;
  virtual void setAttribute(bool readEnable, int baudrate);

  void readEnable() {
    setAttribute(true, baudrate_);
  }
  void readDisable() {
    setAttribute(false, baudrate_);
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
    setAttribute(true, baudrate);
  }

  void open() override;
  void write(const uint8_t* buf, size_t len) override;

  void write(const std::vector<uint8_t>& buf) {
    write(buf.data(), buf.size());
  }
};

class AspeedRS485Device : public UARTDevice {
 public:
  AspeedRS485Device(const std::string& device, int baudrate)
      : UARTDevice(device, baudrate) {}
  void open() override;
};

class LocalEchoUARTDevice : public UARTDevice {
 public:
  LocalEchoUARTDevice(const std::string& device, int baudrate)
      : UARTDevice(device, baudrate) {}
  void write(const uint8_t* buf, size_t len) override;
  void waitWrite() override {}
};

} // namespace rackmon
