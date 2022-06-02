// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <nlohmann/json.hpp>
#include <ctime>
#include <iostream>
#include "Modbus.h"
#include "ModbusCmds.h"
#include "Register.h"

namespace rackmon {

enum class ModbusDeviceMode { ACTIVE = 0, DORMANT = 1 };

class ModbusDevice;

class ModbusSpecialHandler : public SpecialHandlerInfo {
  uint8_t deviceAddress_;
  time_t lastHandleTime_ = 0;
  bool handled_ = false;
  bool canHandle() {
    if (period == -1) {
      return !handled_;
    }
    return getTime() > (lastHandleTime_ + period);
  }

  virtual time_t getTime() {
    return std::time(nullptr);
  }

 public:
  ModbusSpecialHandler(uint8_t deviceAddress) : deviceAddress_(deviceAddress) {}
  virtual ~ModbusSpecialHandler() {}
  void handle(ModbusDevice& dev);
};

// Generic Device information
struct ModbusDeviceInfo {
  static constexpr uint32_t kMaxConsecutiveFailures = 10;
  uint8_t deviceAddress = 0;
  std::string deviceType{"Unknown"};
  uint32_t baudrate = 0;
  ModbusDeviceMode mode = ModbusDeviceMode::ACTIVE;
  uint32_t crcErrors = 0;
  uint32_t timeouts = 0;
  uint32_t miscErrors = 0;
  uint32_t deviceErrors = 0;
  time_t lastActive = 0;
  uint32_t numConsecutiveFailures = 0;

  void incErrors(uint32_t& counter);
  void incTimeouts() {
    incErrors(timeouts);
  }
  void incCRCErrors() {
    incErrors(crcErrors);
  }
  void incMiscErrors() {
    incErrors(miscErrors);
  }
};
void to_json(nlohmann::json& j, const ModbusDeviceInfo& m);

// Device raw register data
struct ModbusDeviceRawData : public ModbusDeviceInfo {
  std::vector<RegisterStore> registerList{};
};
void to_json(nlohmann::json& j, const ModbusDeviceRawData& m);

// Device interpreted register value format
struct ModbusDeviceValueData : public ModbusDeviceInfo {
  std::vector<RegisterStoreValue> registerList{};
};
void to_json(nlohmann::json& j, const ModbusDeviceValueData& m);

class ModbusDevice {
  Modbus& interface_;
  int numCommandRetries_;
  ModbusDeviceRawData info_;
  std::mutex registerListMutex_{};
  std::vector<ModbusSpecialHandler> specialHandlers_{};

  void handleCommandFailure(std::exception& baseException);

 public:
  ModbusDevice(
      Modbus& interface,
      uint8_t deviceAddress,
      const RegisterMap& registerMap,
      int numCommandRetries = 5);
  virtual ~ModbusDevice() {}

  virtual void
  command(Msg& req, Msg& resp, ModbusTime timeout = ModbusTime::zero());

  void readHoldingRegisters(
      uint16_t registerOffset,
      std::vector<uint16_t>& regs,
      ModbusTime timeout = ModbusTime::zero());

  void writeSingleRegister(
      uint16_t registerOffset,
      uint16_t value,
      ModbusTime timeout = ModbusTime::zero());

  void writeMultipleRegisters(
      uint16_t registerOffset,
      std::vector<uint16_t>& value,
      ModbusTime timeout = ModbusTime::zero());

  void readFileRecord(
      std::vector<FileRecord>& records,
      ModbusTime timeout = ModbusTime::zero());

  void reloadRegisters();

  bool isActive() const {
    return info_.mode == ModbusDeviceMode::ACTIVE;
  }

  void setActive();

  time_t lastActive() const {
    return info_.lastActive;
  }

  // Return structured information of the device.
  ModbusDeviceInfo getInfo();

  // Returns raw register data monitored for this device.
  ModbusDeviceRawData getRawData();

  // Returns value formatted register data monitored for this device.
  ModbusDeviceValueData getValueData();
};

} // namespace rackmon
