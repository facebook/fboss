// Copyright 2021-present Facebook. All Rights Reserved.
#include "ModbusDevice.h"
#include <iomanip>
#include <sstream>
#include "Log.h"

using nlohmann::json;

namespace rackmon {

ModbusDevice::ModbusDevice(
    Modbus& interface,
    uint8_t deviceAddress,
    const RegisterMap& registerMap,
    int numCommandRetries)
    : interface_(interface),
      numCommandRetries_(numCommandRetries),
      registerMap_(registerMap) {
  info_.deviceAddress = deviceAddress;
  info_.baudrate = registerMap.baudrate;
  info_.deviceType = registerMap.name;
  info_.parity = registerMap.parity;

  for (auto& it : registerMap.registerDescriptors) {
    info_.registerList.emplace_back(it.second);
  }

  for (const auto& sp : registerMap.specialHandlers) {
    ModbusSpecialHandler hdl(deviceAddress);
    hdl.SpecialHandlerInfo::operator=(sp);
    specialHandlers_.push_back(hdl);
  }
}

void ModbusDevice::handleCommandFailure(std::exception& baseException) {
  if (TimeoutException * ex;
      (ex = dynamic_cast<TimeoutException*>(&baseException)) != nullptr) {
    info_.timeouts++;
  } else if (CRCError * ex;
             (ex = dynamic_cast<CRCError*>(&baseException)) != nullptr) {
    info_.crcErrors++;
  } else if (ModbusError * ex;
             (ex = dynamic_cast<ModbusError*>(&baseException)) != nullptr) {
    // ModbusErrors can happen in normal operation. Do not let
    // it increment numConsecutiveFailures since it should not
    // account as a signal of a device being dormant.
    info_.deviceErrors++;
    return;
  } else if (std::system_error * ex; (ex = dynamic_cast<std::system_error*>(
                                          &baseException)) != nullptr) {
    info_.miscErrors++;
    logError << ex->what() << std::endl;
  } else {
    info_.miscErrors++;
    logError << baseException.what() << std::endl;
  }
  if ((++info_.numConsecutiveFailures) >= kMaxConsecutiveFailures) {
    // If we are in exclusive mode let it continue to
    // fail. We will mark it as dormant when we exit
    // exclusive mode.
    if (!exclusiveMode_) {
      info_.mode = ModbusDeviceMode::DORMANT;
    }
  }
}

std::tuple<uint32_t, Parity> ModbusDevice::getDeviceConfig() {
  std::shared_lock lk(infoMutex_);
  return {info_.baudrate, info_.parity};
}

void ModbusDevice::command(Msg& req, Msg& resp, ModbusTime timeout) {
  size_t reqLen = req.len;
  size_t respLen = resp.len;
  // Try executing the command, if errors, catch the error
  // to maintain stats on types of errors and re-throw (on
  // the last retry) in case the user wants to handle them
  // in a special way.
  int numRetries = exclusiveMode_ ? 1 : numCommandRetries_;
  auto [baudrate, parity] = getDeviceConfig();
  for (int retries = 0; retries < numRetries; retries++) {
    try {
      interface_.command(req, resp, baudrate, timeout, parity);
      std::unique_lock lk(infoMutex_);
      info_.numConsecutiveFailures = 0;
      info_.lastActive = getCurrentTime();
      break;
    } catch (std::exception& ex) {
      std::unique_lock lk(infoMutex_);
      handleCommandFailure(ex);
      if (retries == (numRetries - 1)) {
        throw;
      }
      req.len = reqLen;
      resp.len = respLen;
    }
  }
}

void ModbusDevice::readHoldingRegisters(
    uint16_t registerOffset,
    std::vector<uint16_t>& regs,
    ModbusTime timeout) {
  ReadHoldingRegistersReq req(info_.deviceAddress, registerOffset, regs.size());
  ReadHoldingRegistersResp resp(info_.deviceAddress, regs);
  command(req, resp, timeout);
}

void ModbusDevice::writeSingleRegister(
    uint16_t registerOffset,
    uint16_t value,
    ModbusTime timeout) {
  WriteSingleRegisterReq req(info_.deviceAddress, registerOffset, value);
  WriteSingleRegisterResp resp(info_.deviceAddress, registerOffset);
  command(req, resp, timeout);
}

void ModbusDevice::writeMultipleRegisters(
    uint16_t registerOffset,
    std::vector<uint16_t>& value,
    ModbusTime timeout) {
  WriteMultipleRegistersReq req(info_.deviceAddress, registerOffset);
  for (uint16_t val : value) {
    req << val;
  }
  WriteMultipleRegistersResp resp(
      info_.deviceAddress, registerOffset, value.size());
  command(req, resp, timeout);
}

void ModbusDevice::readFileRecord(
    std::vector<FileRecord>& records,
    ModbusTime timeout) {
  ReadFileRecordReq req(info_.deviceAddress, records);
  ReadFileRecordResp resp(info_.deviceAddress, records);
  command(req, resp, timeout);
}

void ModbusDevice::forceReloadRegister(
    RegisterStore& registerStore,
    time_t reloadTime) {
  uint16_t registerOffset = registerStore.regAddr();
  try {
    std::vector<uint16_t> value(registerStore.length(), 0);
    readHoldingRegisters(registerOffset, value);
    registerStore.setRegister(value.begin(), value.end(), reloadTime);
  } catch (ModbusError& e) {
    if (e.errorCode == ModbusErrorCode::ILLEGAL_DATA_ADDRESS) {
      logInfo << "DEV:0x" << std::hex << int(info_.deviceAddress)
              << " ReadReg 0x" << std::hex << registerOffset << ' '
              << registerStore.name()
              << " unsupported. Disabled from monitoring" << std::endl;
      registerStore.disable();
    } else {
      logInfo << "DEV:0x" << std::hex << int(info_.deviceAddress)
              << " ReadReg 0x" << std::hex << registerOffset << ' '
              << registerStore.name() << " caught: " << e.what() << std::endl;
    }
  } catch (std::exception& e) {
    logInfo << "DEV:0x" << std::hex << int(info_.deviceAddress) << " ReadReg 0x"
            << std::hex << registerOffset << ' ' << registerStore.name()
            << " caught: " << e.what() << std::endl;
  }
}

void ModbusDevice::forceReloadPlan() {
  reloadPlan_.clear();
  time_t reloadTime = getCurrentTime();
  for (auto& reg : info_.registerList) {
    forceReloadRegister(reg, reloadTime);
    RegisterStoreSpan::buildRegisterSpanList(reloadPlan_, reg);
  }
}

bool ModbusDevice::reloadRegisterSpan(
    RegisterStoreSpan& span,
    bool singleShot) {
  time_t reloadTime = getCurrentTime();
  if (!singleShot && !span.reloadPending(reloadTime)) {
    return false;
  }
  uint16_t registerOffset = span.getSpanAddress();
  try {
    auto& data = span.beginReloadSpan();
    readHoldingRegisters(registerOffset, data);
    span.endReloadSpan(reloadTime);
    return true;
  } catch (ModbusError& e) {
    logInfo << "DEV:0x" << std::hex << int(info_.deviceAddress) << " ReadReg 0x"
            << std::hex << registerOffset << " Len: " << span.length()
            << " caught: " << e.what() << std::endl;
    if (e.errorCode == ModbusErrorCode::ILLEGAL_DATA_ADDRESS) {
      // TODO we might need to reform a plan.
    }
  } catch (std::exception& e) {
    logInfo << "DEV:0x" << std::hex << int(info_.deviceAddress) << " ReadReg 0x"
            << std::hex << registerOffset << " Len: " << span.length()
            << " caught: " << e.what() << std::endl;
  }
  return false;
}

void ModbusDevice::reloadAllRegisters() {
  // If the number of consecutive failures has exceeded
  // a threshold, mark the device as dormant.
  for (auto& specialHandler : specialHandlers_) {
    // Break early, if we are entering exclusive mode
    if (exclusiveMode_) {
      break;
    }
    specialHandler.handle(*this);
  }
  bool singleShot = singleShotReload_;
  singleShotReload_ = false;
  if (singleShot) {
    forceReloadPlan();
    return;
  }
  for (auto& plan : reloadPlan_) {
    // Break early, if we are entering exclusive mode
    if (exclusiveMode_) {
      break;
    }
    if (reloadRegisterSpan(plan, singleShot)) {
      // Release thread to allow for higher priority tasks to execute.
      std::this_thread::yield();
    }
  }
}

void ModbusDevice::setActive() {
  std::unique_lock lk(infoMutex_);
  // Enable any disabled registers. Assumption is
  // that the device might be unplugged and replugged
  // with a newer version. Thus, prepare for the
  // newer register set.
  for (auto& registerStore : info_.registerList) {
    registerStore.enable();
  }
  // Clear the num failures so we consider it active.
  info_.numConsecutiveFailures = 0;
  info_.mode = ModbusDeviceMode::ACTIVE;
  // Force read all registers on next reload
  singleShotReload_ = true;
}

ModbusDeviceRawData ModbusDevice::getRawData() {
  std::shared_lock lk(infoMutex_);
  // Makes a deep copy.
  return info_;
}

ModbusDeviceInfo ModbusDevice::getInfo() {
  std::shared_lock lk(infoMutex_);
  return info_;
}

ModbusDeviceValueData ModbusDevice::getValueData(
    const ModbusRegisterFilter& filter,
    bool latestValueOnly) const {
  ModbusDeviceValueData data;
  {
    std::shared_lock lk(infoMutex_);
    data.ModbusDeviceInfo::operator=(info_);
  }
  auto shouldPickRegister = [&filter](const RegisterStore& reg) {
    return !filter || filter.contains(reg.regAddr()) ||
        filter.contains(reg.name());
  };
  for (const auto& reg : info_.registerList) {
    if (shouldPickRegister(reg)) {
      if (latestValueOnly) {
        data.registerList.emplace_back(reg.regAddr(), reg.name());
        data.registerList.back().history.emplace_back(reg.back());
      } else {
        data.registerList.emplace_back(reg);
      }
    }
  }
  return data;
}

void ModbusDevice::forceReloadRegisters(const ModbusRegisterFilter& filter) {
  auto shouldPickRegister = [&filter](const RegisterStore& reg) {
    return !filter || filter.contains(reg.regAddr()) ||
        filter.contains(reg.name());
  };
  std::vector<RegisterStoreSpan> regSpans{};
  for (auto& reg : info_.registerList) {
    if (shouldPickRegister(reg)) {
      bool added = RegisterStoreSpan::buildRegisterSpanList(regSpans, reg);
      if (!added) {
        logError << "reload:: Not including register: " << reg.name()
                 << std::endl;
      }
    }
  }
  for (auto& span : regSpans) {
    if (!reloadRegisterSpan(span, true)) {
      logError << "reload:: Reload failed at address: "
               << +span.getSpanAddress() << std::endl;
    }
  }
}

static std::string commandOutput(const std::string& shell) {
  std::array<char, 128> buffer;
  std::string result;
  auto pipe_close = [](auto fd) { (void)pclose(fd); };
  std::unique_ptr<FILE, decltype(pipe_close)> pipe(
      popen(shell.c_str(), "r"), pipe_close);
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
    result += buffer.data();
  }
  return result;
}

void ModbusSpecialHandler::handle(ModbusDevice& dev) {
  // Check if it is time to handle.
  if (!canHandle()) {
    return;
  }
  std::string strValue{};
  WriteMultipleRegistersReq req(deviceAddress_, reg);
  if (info.shell) {
    // The command is from the JSON configuration.
    // TODO, we currently only have need to set the
    // current UNIX time. If we want to avoid shell,
    // we might need a different way to generalize
    // this.
    strValue = commandOutput(info.shell.value());
  } else if (info.value) {
    strValue = info.value.value();
  } else {
    logError << "NULL action ignored" << std::endl;
    return;
  }
  if (info.interpret == RegisterValueType::INTEGER) {
    int32_t ival = std::stoi(strValue);
    if (len == 1) {
      req << uint16_t(ival);
    } else if (len == 2) {
      req << uint32_t(ival);
    } else {
      logError << "Value truncated to 32bits" << std::endl;
      req << uint32_t(ival);
    }
  } else if (info.interpret == RegisterValueType::STRING) {
    for (char c : strValue) {
      req << uint8_t(c);
    }
  }
  WriteMultipleRegistersResp resp(deviceAddress_, reg, len);
  try {
    dev.command(req, resp);
  } catch (ModbusError& e) {
    if (e.errorCode == ModbusErrorCode::ILLEGAL_DATA_ADDRESS ||
        e.errorCode == ModbusErrorCode::ILLEGAL_FUNCTION) {
      logInfo << "DEV:0x" << std::hex << +deviceAddress_
              << " Special Handler at 0x" << std::hex << +reg << ' '
              << " unsupported. Disabled from future updates" << std::endl;
      period = -1;
    } else {
      logInfo << "DEV:0x" << std::hex << +deviceAddress_
              << " Special Handler at 0x" << std::hex << +reg << ' '
              << " caught: " << e.what() << std::endl;
    }
  } catch (std::exception& e) {
    logInfo << "DEV:0x" << std::hex << +deviceAddress_
            << " Special Handler at 0x" << std::hex << +reg << ' '
            << " caught: " << e.what() << std::endl;
  }
  lastHandleTime_ = getTime();
  handled_ = true;
}

NLOHMANN_JSON_SERIALIZE_ENUM(
    ModbusDeviceMode,
    {{ModbusDeviceMode::ACTIVE, "ACTIVE"},
     {ModbusDeviceMode::DORMANT, "DORMANT"}})

void to_json(json& j, const ModbusDeviceInfo& m) {
  j["devAddress"] = m.deviceAddress;
  j["deviceType"] = m.deviceType;
  j["crcErrors"] = m.crcErrors;
  j["timeouts"] = m.timeouts;
  j["miscErrors"] = m.miscErrors;
  j["baudrate"] = m.baudrate;
  j["mode"] = m.mode;
}

// Legacy JSON format.
void to_json(json& j, const ModbusDeviceRawData& m) {
  j["addr"] = m.deviceAddress;
  j["crc_fails"] = m.crcErrors;
  j["timeouts"] = m.timeouts;
  j["misc_fails"] = m.miscErrors;
  j["mode"] = m.mode;
  j["baudrate"] = m.baudrate;
  j["deviceType"] = m.deviceType;
  j["now"] = std::time(nullptr);
  j["ranges"] = m.registerList;
}

// v2.0 JSON Format.
void to_json(json& j, const ModbusDeviceValueData& m) {
  const ModbusDeviceInfo& devInfo = m;
  j["devInfo"] = devInfo;
  j["regList"] = m.registerList;
}

} // namespace rackmon
