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

void Rackmon::loadInterface(const nlohmann::json& config) {
  if (scanThread_ != nullptr || monitorThread_ != nullptr) {
    throw std::runtime_error("Cannot load configuration when started");
  }
  if (interfaces_.size() > 0) {
    throw std::runtime_error("Interfaces already loaded");
  }
  for (const auto& ifaceConf : config["interfaces"]) {
    interfaces_.push_back(makeInterface());
    interfaces_.back()->initialize(ifaceConf);
  }
}

void Rackmon::loadRegisterMap(const nlohmann::json& config) {
  if (scanThread_ != nullptr || monitorThread_ != nullptr) {
    throw std::runtime_error("Cannot load configuration when started");
  }
  registerMapDB_.load(config);
  // Precomputing this makes our scan soooo much easier.
  // its 256 bytes wasted. but worth it. TODO use a
  // interval list with an iterator to waste less bytes.
  for (uint16_t addr = config["address_range"][0];
       addr <= config["address_range"][1];
       ++addr) {
    allPossibleDevAddrs_.push_back(uint8_t(addr));
  }
  nextDeviceToProbe_ = allPossibleDevAddrs_.begin();
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

bool Rackmon::probe(Modbus& interface, uint8_t addr) {
  if (!interface.isPresent()) {
    return false;
  }
  const RegisterMap& rmap = registerMapDB_.at(addr);
  std::vector<uint16_t> v(1);
  try {
    ReadHoldingRegistersReq req(addr, rmap.probeRegister, v.size());
    ReadHoldingRegistersResp resp(addr, v);
    interface.command(req, resp, rmap.defaultBaudrate, kProbeTimeout);
    std::unique_lock lock(devicesMutex_);
    devices_[addr] = std::make_unique<ModbusDevice>(interface, addr, rmap);
    logInfo << std::hex << std::setw(2) << std::setfill('0') << "Found "
            << int(addr) << " on " << interface.name() << std::endl;
    return true;
  } catch (std::exception& e) {
    return false;
  }
}

bool Rackmon::probe(uint8_t addr) {
  // We do not support the same address
  // on multiple interfaces.
  return std::any_of(
      interfaces_.begin(), interfaces_.end(), [this, addr](auto& iface) {
        return probe(*iface, addr);
      });
}

std::vector<uint8_t> Rackmon::inspectDormant() {
  std::vector<uint8_t> ret{};
  std::shared_lock lock(devicesMutex_);
  for (const auto& it : devices_) {
    if (it.second->isActive()) {
      continue;
    }
    time_t curr = getTime();
    // If its more than 300s since last activity, start probing it.
    // change to something larger if required.
    if ((it.second->lastActive() + kDormantMinInactiveTime) < curr) {
      const RegisterMap& rmap = registerMapDB_.at(it.first);
      uint16_t probe = rmap.probeRegister;
      std::vector<uint16_t> v(1);
      try {
        uint8_t addr = it.first;
        it.second->readHoldingRegisters(probe, v);
        ret.push_back(addr);
      } catch (...) {
        continue;
      }
    }
  }
  return ret;
}

void Rackmon::recoverDormant() {
  std::vector<uint8_t> candidates = inspectDormant();
  for (auto& addr : candidates) {
    std::unique_lock lock(devicesMutex_);
    devices_.at(addr)->setActive();
  }
}

void Rackmon::monitor(void) {
  std::shared_lock lock(devicesMutex_);
  for (const auto& dev_it : devices_) {
    if (!dev_it.second->isActive()) {
      continue;
    }
    dev_it.second->reloadRegisters();
  }
  lastMonitorTime_ = std::time(nullptr);
}

bool Rackmon::isDeviceKnown(uint8_t addr) {
  std::shared_lock lk(devicesMutex_);
  return devices_.find(addr) != devices_.end();
}

ModbusDevice& Rackmon::getModbusDevice(uint8_t addr) {
  std::stringstream err;
  auto it = devices_.find(addr);
  if (it == devices_.end()) {
    err << "No device found at 0x" << std::hex << +addr
        << " during probe sequence";
    throw std::out_of_range(err.str());
  }
  auto& d = *it->second;
  if (!d.isActive()) {
    err << "Device 0x" << std::hex << +addr << " is not active";
    throw std::runtime_error(err.str());
  }
  return d;
}

void Rackmon::fullScan() {
  logInfo << "Starting scan of all devices" << std::endl;
  for (auto& addr : allPossibleDevAddrs_) {
    if (isDeviceKnown(addr)) {
      continue;
    }
    for (int i = 0; i < kScanNumRetry; i++) {
      if (probe(addr)) {
        break;
      }
    }
  }
}

void Rackmon::scan() {
  // Circular iterator.
  if (reqForceScan_.load()) {
    fullScan();
    reqForceScan_ = false;
    return;
  }

  // Probe for the address only if we already dont know it.
  if (!isDeviceKnown(*nextDeviceToProbe_)) {
    probe(*nextDeviceToProbe_);
    lastScanTime_ = std::time(nullptr);
  }

  // Try and recover dormant devices
  recoverDormant();
  if (++nextDeviceToProbe_ == allPossibleDevAddrs_.end()) {
    nextDeviceToProbe_ = allPossibleDevAddrs_.begin();
  }
}

std::unique_ptr<PollThread<Rackmon>> Rackmon::makeThread(
    std::function<void(Rackmon*)> func,
    PollThreadTime interval) {
  return std::make_unique<PollThread<Rackmon>>(func, this, interval);
}

void Rackmon::start(PollThreadTime interval) {
  if (scanThread_ != nullptr || monitorThread_ != nullptr) {
    throw std::runtime_error("Already running");
  }
  for (auto& dev_it : devices_) {
    dev_it.second->setExclusiveMode(false);
  }
  scanThread_ = makeThread(&Rackmon::scan, interval);
  scanThread_->start();
  monitorThread_ = makeThread(&Rackmon::monitor, interval);
  monitorThread_->start();
}

void Rackmon::stop() {
  for (auto& dev_it : devices_) {
    dev_it.second->setExclusiveMode(true);
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

void Rackmon::rawCmd(Request& req, Response& resp, ModbusTime timeout) {
  uint8_t addr = req.addr;
  RACKMON_PROFILE_SCOPE(raw_cmd, "rawcmd::" + std::to_string(int(req.addr)));
  std::shared_lock lock(devicesMutex_);

  getModbusDevice(addr).command(req, resp, timeout);
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

  getModbusDevice(deviceAddress)
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

  getModbusDevice(deviceAddress)
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

  getModbusDevice(deviceAddress)
      .writeMultipleRegisters(registerOffset, values, timeout);
}

void Rackmon::readFileRecord(
    uint8_t deviceAddress,
    std::vector<FileRecord>& records,
    ModbusTime timeout) {
  RACKMON_PROFILE_SCOPE(
      raw_cmd, "ReadFile::" + std::to_string(int(deviceAddress)));
  std::shared_lock lock(devicesMutex_);
  getModbusDevice(deviceAddress).readFileRecord(records, timeout);
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
  auto isInFilter = [&devFilter](const ModbusDevice& dev) {
    return devFilter.contains(dev.getDeviceAddress()) ||
        devFilter.contains(dev.getDeviceType());
  };
  data.clear();
  std::shared_lock lock(devicesMutex_);
  if (!devFilter) {
    std::transform(
        devices_.begin(),
        devices_.end(),
        std::back_inserter(data),
        [&regFilter, latestValueOnly](auto& kv) {
          return kv.second->getValueData(regFilter, latestValueOnly);
        });
  } else {
    for (auto& kv : devices_) {
      ModbusDevice& dev = *kv.second;
      if (isInFilter(dev)) {
        data.push_back(dev.getValueData(regFilter, latestValueOnly));
      }
    }
  }
}

} // namespace rackmon
