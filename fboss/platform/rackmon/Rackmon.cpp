// Copyright 2021-present Facebook. All Rights Reserved.
#include "Rackmon.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include "Log.h"

#if (defined(__llvm__) && (__clang_major__ < 9)) || \
    (!defined(__llvm__) && (__GNUC__ < 8))
#include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif

using nlohmann::json;
using namespace std::literals;

namespace rackmon {

bool ModbusDeviceFilter::contains(const ModbusDevice& dev) const {
  // If neither is provided, its considered as a
  // shortcut of "all".
  if (!addrFilter && !typeFilter) {
    return true;
  }
  if (addrFilter &&
      addrFilter->find(dev.getDeviceAddress()) != addrFilter->end()) {
    return true;
  }
  if (typeFilter &&
      typeFilter->find(dev.getDeviceType()) != typeFilter->end()) {
    return true;
  }
  return false;
}

void Rackmon::loadInterface(const nlohmann::json& config) {
  std::shared_lock lk(threadMutex_);
  if (scanThread_ != nullptr || monitorThread_ != nullptr) {
    throw std::runtime_error("Cannot load configuration when started");
  }
  if (!interfaces_.empty()) {
    throw std::runtime_error("Interfaces already loaded");
  }
  for (const auto& ifaceConf : config["interfaces"]) {
    interfaces_.push_back(makeInterface());
    interfaces_.back()->initialize(ifaceConf);
  }
}

void Rackmon::loadRegisterMap(const nlohmann::json& config) {
  std::shared_lock lk(threadMutex_);
  if (scanThread_ != nullptr || monitorThread_ != nullptr) {
    throw std::runtime_error("Cannot load configuration when started");
  }
  registerMapDB_.load(config);
  // Precomputing this makes our scan soooo much easier.
  // its 256 bytes wasted. but worth it. TODO use a
  // interval list with an iterator to waste less bytes.
  for (auto range : config["address_range"]) {
    for (uint16_t addr = range[0]; addr <= range[1]; ++addr) {
      allPossibleDevAddrs_.push_back({uint8_t(addr)});
    }
  }
  nextDeviceToProbe_ = allPossibleDevAddrs_.begin();
  monitorInterval_ = std::chrono::seconds(registerMapDB_.minMonitorInterval());
}

void Rackmon::load(const std::string& confPath, const std::string& regmapDir) {
  auto getJSON = [](const std::string& fileName) {
    std::ifstream ifs(fileName);
    json contents;
    try {
      ifs >> contents;
    } catch (const nlohmann::json::parse_error& ex) {
      logError << "Error loading: " << fileName << " byte: " << ex.byte
               << std::endl;
      throw;
    }
    ifs.close();
    return contents;
  };
  loadInterface(getJSON(confPath));

  for (auto const& dir_entry : std::filesystem::directory_iterator{regmapDir}) {
    loadRegisterMap(getJSON(dir_entry.path().string()));
  }
}

bool Rackmon::probe(Modbus& interface, DeviceLocation key) {
  if (!interface.isPresent()) {
    return false;
  }
  for (auto it = registerMapDB_.find(key.addr); it != registerMapDB_.end();
       ++it) {
    const auto& rmap = *it;
    std::vector<uint16_t> v(1);
    try {
      ReadHoldingRegistersReq req(key.addr, rmap.probeRegister, v.size());
      ReadHoldingRegistersResp resp(key.addr, v);
      interface.command(req, resp, rmap.baudrate, kProbeTimeout, rmap.parity);
      {
        std::unique_lock lock(devicesMutex_);
        devices_[key] =
            std::make_unique<ModbusDevice>(interface, key.addr, rmap);
      }
      logInfo << std::setw(2) << std::setfill('0') << "Found " << key << " on "
              << interface.name() << std::endl;
      return true;
    } catch (std::exception&) {
      // Exceptions are expected for unfound addresses.
    }
  }
  return false;
}

bool Rackmon::probe(DeviceLocation key) {
  // We do not support the same address
  // on multiple interfaces.
  return std::any_of(
      interfaces_.begin(), interfaces_.end(), [this, key](const auto& iface) {
        return probe(*iface, key);
      });
}

std::vector<DeviceLocation> Rackmon::inspectDormant() {
  std::vector<DeviceLocation> ret{};
  std::shared_lock lock(devicesMutex_);
  for (const auto& it : devices_) {
    if (it.second->isActive()) {
      continue;
    }
    time_t curr = getTime();
    // If its more than 300s since last activity, start probing it.
    // change to something larger if required.
    if ((it.second->lastActive() + kDormantMinInactiveTime) < curr) {
      const RegisterMap& rmap = it.second->getRegisterMap();
      uint16_t probe = rmap.probeRegister;
      std::vector<uint16_t> v(1);
      try {
        it.second->readHoldingRegisters(probe, v);
        ret.push_back(it.first);
      } catch (...) {
        continue;
      }
    }
  }
  return ret;
}

void Rackmon::recoverDormant() {
  const std::vector<DeviceLocation> dormant = inspectDormant();
  for (const auto& key : dormant) {
    std::unique_lock lock(devicesMutex_);
    devices_.at(key)->setActive();
  }
}

void Rackmon::monitor() {
  std::shared_lock lock(devicesMutex_);
  for (const auto& dev_it : devices_) {
    if (!dev_it.second->isActive()) {
      continue;
    }
    dev_it.second->reloadAllRegisters();
  }
  lastMonitorTime_ = std::time(nullptr);
}

bool Rackmon::isDeviceKnown(DeviceLocation key) {
  std::shared_lock lk(devicesMutex_);
  return devices_.find(key) != devices_.end();
}

ModbusDevice& Rackmon::getModbusDevice(DeviceLocation key) {
  std::stringstream err;
  auto it = devices_.find(key);
  if (it == devices_.end()) {
    err << "No device found at " << key << " during probe sequence";
    throw std::out_of_range(err.str());
  }
  auto& d = *it->second;
  if (!d.isActive()) {
    err << "Device " << key << " is not active";
    throw std::runtime_error(err.str());
  }
  return d;
}

void Rackmon::fullScan() {
  logInfo << "Starting scan of all devices" << std::endl;
  bool atLeastOne = false;
  // Retry the scan loop to ensure we discover any flaky
  // devices which might have missed the first loop.
  for (int i = 0; i < kScanNumRetry; i++) {
    for (const auto& key : allPossibleDevAddrs_) {
      if (isDeviceKnown(key)) {
        continue;
      }
      if (reqForceScan_.load() == false) {
        logWarn << "Full scan aborted" << std::endl;
        return;
      }
      if (probe(key)) {
        atLeastOne = true;
      }
    }
  }
  logInfo << "Finished scan of all devices" << std::endl;
  // When scan is complete, request for a monitor.
  if (atLeastOne) {
    std::shared_lock lk(threadMutex_);
    if (monitorThread_) {
      monitorThread_->tick(true);
    }
  }
  reqForceScan_ = false;
}

void Rackmon::scan() {
  // Circular iterator.
  if (reqForceScan_.load()) {
    fullScan();
    return;
  }

  // Probe for the address only if we already dont know it.
  if (!isDeviceKnown(*nextDeviceToProbe_)) {
    if (probe(*nextDeviceToProbe_)) {
      std::shared_lock lk(threadMutex_);
      if (monitorThread_) {
        monitorThread_->tick(true);
      }
    }
    lastScanTime_ = std::time(nullptr);
  }

  // Try and recover dormant devices
  recoverDormant();
  if (++nextDeviceToProbe_ == allPossibleDevAddrs_.end()) {
    nextDeviceToProbe_ = allPossibleDevAddrs_.begin();
  }
}

std::shared_ptr<PollThread<Rackmon>> Rackmon::makeThread(
    std::function<void(Rackmon*)> func,
    PollThreadTime interval) {
  return std::make_shared<PollThread<Rackmon>>(func, this, interval);
}

void Rackmon::start(PollThreadTime interval) {
  std::unique_lock lk(threadMutex_);
  logInfo << "Start was requested" << std::endl;
  if (scanThread_ != nullptr || monitorThread_ != nullptr) {
    throw std::runtime_error("Already running");
  }
  for (auto& dev_it : devices_) {
    dev_it.second->setExclusiveMode(false);
  }
  scanThread_ = makeThread(&Rackmon::scan, interval);
  scanThread_->start();
  monitorThread_ = makeThread(&Rackmon::monitor, monitorInterval_);
  monitorThread_->start();
}

void Rackmon::stop(bool forceStop) {
  std::unique_lock lk(threadMutex_);
  logInfo << "Stop was requested" << std::endl;
  for (auto& dev_it : devices_) {
    dev_it.second->setExclusiveMode(true);
  }
  if (forceStop) {
    reqForceScan_ = false;
  }
  // TODO We probably need a timer to ensure we
  // are not waiting here forever.
  if (monitorThread_ != nullptr) {
    monitorThread_->stop();
    monitorThread_ = nullptr;
  }
  if (scanThread_ != nullptr) {
    scanThread_->stop();
    scanThread_ = nullptr;
  }
}

void Rackmon::forceScan() {
  logInfo << "Force Scan was requested" << std::endl;
  reqForceScan_ = true;
  std::shared_lock lk(threadMutex_);
  if (scanThread_) {
    scanThread_->tick(true);
  }
}

void Rackmon::rawCmd(Request& req, Response& resp, ModbusTime timeout) {
  uint8_t addr = req.addr;
  RACKMON_PROFILE_SCOPE(raw_cmd, "rawcmd::" + std::to_string(int(req.addr)));
  std::shared_lock lock(devicesMutex_);

  getModbusDevice({addr}).command(req, resp, timeout);
  // Add back the CRC removed by validate.
  resp.len += 2;
}

void Rackmon::readHoldingRegisters(
    uint8_t deviceAddress,
    uint16_t registerOffset,
    std::vector<uint16_t>& registerContents,
    ModbusTime timeout) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "readRegs::" + std::to_string(int(deviceAddress)));
  std::shared_lock lock(devicesMutex_);

  getModbusDevice({deviceAddress})
      .readHoldingRegisters(registerOffset, registerContents, timeout);
}

void Rackmon::writeSingleRegister(
    uint8_t deviceAddress,
    uint16_t registerOffset,
    uint16_t value,
    ModbusTime timeout) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "writeReg::" + std::to_string(int(deviceAddress)));
  std::shared_lock lock(devicesMutex_);

  getModbusDevice({deviceAddress})
      .writeSingleRegister(registerOffset, value, timeout);
}

void Rackmon::writeMultipleRegisters(
    uint8_t deviceAddress,
    uint16_t registerOffset,
    std::vector<uint16_t>& values,
    ModbusTime timeout) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "writeRegs::" + std::to_string(int(deviceAddress)));
  std::shared_lock lock(devicesMutex_);

  getModbusDevice({deviceAddress})
      .writeMultipleRegisters(registerOffset, values, timeout);
}

void Rackmon::readFileRecord(
    uint8_t deviceAddress,
    std::vector<FileRecord>& records,
    ModbusTime timeout) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "ReadFile::" + std::to_string(int(deviceAddress)));
  std::shared_lock lock(devicesMutex_);
  getModbusDevice({deviceAddress}).readFileRecord(records, timeout);
}

std::vector<ModbusDeviceInfo> Rackmon::listDevices() const {
  std::shared_lock lock(devicesMutex_);
  std::vector<ModbusDeviceInfo> devices;
  std::transform(
      devices_.begin(),
      devices_.end(),
      std::back_inserter(devices),
      [](auto& kv) { return kv.second->getInfo(); });
  return devices;
}

void Rackmon::getRawData(std::vector<ModbusDeviceRawData>& data) const {
  data.clear();
  std::shared_lock lock(devicesMutex_);
  std::transform(
      devices_.begin(), devices_.end(), std::back_inserter(data), [](auto& kv) {
        return kv.second->getRawData();
      });
}

void Rackmon::getValueData(
    std::vector<ModbusDeviceValueData>& data,
    const ModbusDeviceFilter& devFilter,
    const ModbusRegisterFilter& regFilter,
    bool latestValueOnly) const {
  data.clear();
  std::shared_lock lock(devicesMutex_);
  for (const auto& kv : devices_) {
    const ModbusDevice& dev = *kv.second;
    if (devFilter.contains(dev)) {
      data.push_back(dev.getValueData(regFilter, latestValueOnly));
    }
  }
}

void Rackmon::reload(
    const ModbusDeviceFilter& devFilter,
    const ModbusRegisterFilter& regFilter) {
  std::shared_lock lock(devicesMutex_);
  for (const auto& kv : devices_) {
    ModbusDevice& dev = *kv.second;
    if (devFilter.contains(dev)) {
      logInfo << "Force Reloading: " << +dev.getDeviceAddress() << std::endl;
      dev.forceReloadRegisters(regFilter);
    }
  }
}

} // namespace rackmon
