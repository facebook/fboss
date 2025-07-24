// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <atomic>
#include <optional>
#include <set>
#include <shared_mutex>
#include <thread>
#include "Modbus.h"
#include "ModbusDevice.h"
#include "PollThread.h"

namespace rackmon {

struct ModbusDeviceFilter {
  std::optional<std::set<uint8_t>> addrFilter{};
  std::optional<std::set<std::string>> typeFilter{};
  bool contains(const ModbusDevice& dev) const;
};

class Rackmon {
  static constexpr int kScanNumRetry = 3;
  static constexpr time_t kDormantMinInactiveTime = 300;
  static constexpr ModbusTime kProbeTimeout = std::chrono::milliseconds(70);
  std::shared_mutex threadMutex_{};
  std::shared_ptr<PollThread<Rackmon>> monitorThread_;
  std::shared_ptr<PollThread<Rackmon>> scanThread_;
  // Has to be before defining active or dormant devices
  // to ensure users get destroyed before the interface.
  std::vector<std::unique_ptr<Modbus>> interfaces_{};
  RegisterMapDatabase registerMapDB_{};

  mutable std::shared_mutex devicesMutex_{};

  // These devices discovered on actively monitored busses
  std::map<uint8_t, std::unique_ptr<ModbusDevice>> devices_{};

  // contains all the possible address allowed by currently
  // loaded register maps. A majority of these are not expected
  // to exist, but are candidates for a scan.
  std::vector<uint8_t> allPossibleDevAddrs_{};
  std::vector<uint8_t>::iterator nextDeviceToProbe_{};

  // As an optimization, devices are normally scanned one by one
  // This allows someone to initiate a forced full scan.
  // This mimicks a restart of rackmond.
  std::atomic<bool> reqForceScan_ = true;

  // Timestamps of last scan
  time_t lastScanTime_;
  time_t lastMonitorTime_;

  // Interval at which we will monitor all the discovered
  // devices.
  PollThreadTime monitorInterval_ = std::chrono::minutes(3);

  // Probe an interface for the presence of the address.
  bool probe(Modbus& interface, uint8_t addr);

  // Probe for the presence of an address
  bool probe(uint8_t addr);

  // --------- Private Methods --------

  // probe dormant devices and return recovered devices.
  std::vector<uint8_t> inspectDormant();
  // Try and recover dormant devices.
  void recoverDormant();

  virtual time_t getTime() {
    return std::time(nullptr);
  }

  bool isDeviceKnown(uint8_t);

  // Monitor loop. Blocks forever as long as req_stop is true.
  void monitor();

  // Scan all possible devices. Skips active/dormant devices.
  void fullScan();

  // Scan loop. Blocks forever as long as req_stop is true.
  void scan();

 protected:
  // Return the device given address.
  ModbusDevice& getModbusDevice(uint8_t addr);

  PollThread<Rackmon>& getScanThread() {
    if (!scanThread_) {
      throw std::runtime_error("Invalid scanThread state");
    }
    return *scanThread_;
  }

  PollThread<Rackmon>& getMonitorThread() {
    if (!monitorThread_) {
      throw std::runtime_error("Invalid monitorThread state");
    }
    return *monitorThread_;
  }

  virtual std::unique_ptr<Modbus> makeInterface() {
    return std::make_unique<Modbus>();
  }
  const RegisterMapDatabase& getRegisterMapDatabase() const {
    return registerMapDB_;
  }

 public:
  virtual ~Rackmon() {
    stop();
  }

  // Load Interface configuration.
  void loadInterface(const nlohmann::json& config);

  // Load a register map into the internal database.
  void loadRegisterMap(const nlohmann::json& config);

  // Load configuration, preferable before starting, but can be
  // done at any time, but this is a one time only.
  void load(const std::string& confPath, const std::string& regmapDir);

  // Create a worker thread
  virtual std::shared_ptr<PollThread<Rackmon>> makeThread(
      std::function<void(Rackmon*)> func,
      PollThreadTime interval);

  // Start the monitoring/scanning loops
  void start(PollThreadTime interval = std::chrono::minutes(3));
  // Stop the monitoring/scanning loops
  void stop(bool forceStop = true);

  // Force rackmond to do a full scan on the next scan loop.
  void forceScan();

  // Executes the Raw command. Throws an exception on error.
  void rawCmd(Request& req, Response& resp, ModbusTime timeout);

  // Read registers
  void readHoldingRegisters(
      uint8_t deviceAddress,
      uint16_t registerOffset,
      std::vector<uint16_t>& registerContents,
      ModbusTime timeout = ModbusTime::zero());

  // Write Single Register
  void writeSingleRegister(
      uint8_t deviceAddress,
      uint16_t registerOffset,
      uint16_t value,
      ModbusTime timeout = ModbusTime::zero());

  // Write multiple registers
  void writeMultipleRegisters(
      uint8_t deviceAddress,
      uint16_t registerOffset,
      std::vector<uint16_t>& values,
      ModbusTime timeout = ModbusTime::zero());

  // Read File Record
  void readFileRecord(
      uint8_t deviceAddress,
      std::vector<FileRecord>& records,
      ModbusTime timeout = ModbusTime::zero());

  // Get status of devices
  std::vector<ModbusDeviceInfo> listDevices() const;

  // Get monitored data
  void getRawData(std::vector<ModbusDeviceRawData>& data) const;

  // Get value data
  void getValueData(
      std::vector<ModbusDeviceValueData>& data,
      const ModbusDeviceFilter& devFilter = {},
      const ModbusRegisterFilter& regFilter = {},
      bool latestValueOnly = false) const;

  void reload(
      const ModbusDeviceFilter& devFilter,
      const ModbusRegisterFilter& regFilter);
};

} // namespace rackmon
