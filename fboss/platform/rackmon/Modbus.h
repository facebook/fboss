// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <nlohmann/json.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include "Msg.h"
#include "PollThread.h"
#include "UARTDevice.h"

namespace rackmon {

using ModbusTime = std::chrono::milliseconds;
class Modbus {
  std::string devicePath_{};
  std::unique_ptr<UARTDevice> device_ = nullptr;
  std::mutex deviceMutex_{};
  std::set<uint8_t> ignoredAddrs_ = {};
  uint32_t defaultBaudrate_ = 0;
  ModbusTime defaultTimeout_ = ModbusTime::zero();
  ModbusTime minDelay_ = ModbusTime::zero();
  std::atomic<bool> deviceValid_{false};
  bool debug_{false};
  std::chrono::seconds healthCheckInterval_ = std::chrono::seconds(600);
  std::unique_ptr<PollThread<Modbus>> healthCheckThread_{};

  void healthCheck();
  bool openDevice();
  void closeDevice();

 protected:
  PollThread<Modbus>& getHealthCheckThread() {
    return *healthCheckThread_;
  }

 public:
  explicit Modbus() {}
  virtual ~Modbus() {
    if (healthCheckThread_) {
      healthCheckThread_->stop();
    }
    if (device_) {
      device_->close();
    }
  }

  uint32_t getDefaultBaudrate() const {
    return defaultBaudrate_;
  }
  const std::string& name() const {
    return devicePath_;
  }

  virtual std::unique_ptr<UARTDevice> makeDevice(
      const std::string& deviceType,
      const std::string& devicePath,
      uint32_t baudrate);

  virtual void initialize(const nlohmann::json& j);

  virtual void command(
      Msg& req,
      Msg& resp,
      uint32_t baudrate = 0,
      ModbusTime timeout = ModbusTime::zero(),
      Parity parity = Parity::EVEN);

  virtual bool isPresent() {
    return deviceValid_.load();
  }
};

} // namespace rackmon
