// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <nlohmann/json.hpp>
#include <ctime>
#include <iostream>
#include <optional>
#include <set>
#include "Modbus.h"
#include "ModbusCmds.h"
#include "Register.h"

namespace rackmon {

enum class ModbusDeviceMode { ACTIVE = 0, DORMANT = 1 };

struct ModbusRegisterFilter {
  std::optional<std::set<uint16_t>> addrFilter{};
  std::optional<std::set<std::string>> nameFilter{};
  operator bool() const {
    return addrFilter || nameFilter;
  }
  bool contains(uint16_t addr) const {
    return addrFilter && addrFilter->find(addr) != addrFilter->end();
  }
  bool contains(const std::string& name) const {
    return nameFilter && nameFilter->find(name) != nameFilter->end();
  }
};

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
  uint8_t deviceAddress = 0;
  std::string deviceType{"Unknown"};
  uint32_t baudrate = 0;
  uint32_t preferredBaudrate = 0;
  uint32_t defaultBaudrate = 0;
  ModbusDeviceMode mode = ModbusDeviceMode::ACTIVE;
  uint32_t crcErrors = 0;
  uint32_t timeouts = 0;
  uint32_t miscErrors = 0;
  uint32_t deviceErrors = 0;
  time_t lastActive = 0;
  uint32_t numConsecutiveFailures = 0;
  Parity parity = Parity::EVEN;
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
  static constexpr uint32_t kMaxConsecutiveFailures = 10;
  Modbus& interface_;
  int numCommandRetries_;
  ModbusDeviceRawData info_;
  std::vector<ModbusSpecialHandler> specialHandlers_{};
  const BaudrateConfig& baudConfig_;
  bool setBaudEnabled_ = true;
  bool singleShotReload_ = false;
  std::atomic<bool> exclusiveMode_{false};

  void handleCommandFailure(std::exception& baseException);

  void setBaudrate(uint32_t baud);
  void setDefaultBaudrate() {
    setBaudrate(info_.defaultBaudrate);
  }
  void setPreferredBaudrate() {
    setBaudrate(info_.preferredBaudrate);
  }

  bool reloadRegister(RegisterStore& registerStore, bool singleShot);

 protected:
  virtual time_t getCurrentTime() {
    return std::time(nullptr);
  }

 public:
  ModbusDevice(
      Modbus& interface,
      uint8_t deviceAddress,
      const RegisterMap& registerMap,
      int numCommandRetries = 5);
  virtual ~ModbusDevice() {
    setDefaultBaudrate();
  }

  virtual void
  command(Msg& req, Msg& resp, ModbusTime timeout = ModbusTime::zero());

  uint8_t getDeviceAddress() const {
    return info_.deviceAddress;
  }

  const std::string& getDeviceType() const {
    return info_.deviceType;
  }

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

  void setExclusiveMode(bool enable) {
    exclusiveMode_ = enable;
    // When disabling exclusive mode, enable
    // single shot reload.
    singleShotReload_ = !enable;
  }

  // Return structured information of the device.
  ModbusDeviceInfo getInfo();

  // Returns raw register data monitored for this device.
  ModbusDeviceRawData getRawData();

  // Returns value formatted register data monitored for this device.
  ModbusDeviceValueData getValueData(
      const ModbusRegisterFilter& filter = {},
      bool latestValueOnly = false) const;
};

} // namespace rackmon
