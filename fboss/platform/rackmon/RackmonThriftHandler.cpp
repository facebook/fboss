/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/rackmon/RackmonThriftHandler.h"
#include "fboss/platform/rackmon/RackmonConfig.h"

namespace rackmonsvc {

ModbusDeviceInfo ThriftHandler::transformModbusDeviceInfo(
    const rackmon::ModbusDeviceInfo& source) {
  ModbusDeviceInfo target;
  target.devAddress() = source.deviceAddress;
  target.crcErrors() = source.crcErrors;
  target.miscErrors() = source.miscErrors;
  target.baudrate() = source.baudrate;
  // TODO interpret string source.deviceType to support ORv3 devices.
  target.deviceType() = ModbusDeviceType::ORV2_PSU;
  target.mode() = source.mode == rackmon::ModbusDeviceMode::ACTIVE
      ? ModbusDeviceMode::ACTIVE
      : ModbusDeviceMode::DORMANT;
  return target;
}

ModbusRegisterValue ThriftHandler::transformRegisterValue(
    const rackmon::RegisterValue& value) {
  ModbusRegisterValue target{};

  target.timestamp() = value.timestamp;
  switch (value.type) {
    case rackmon::RegisterValueType::HEX: {
      target.type() = RegisterValueType::RAW;
      std::vector<int8_t> conv(value.value.hexValue.size());
      std::copy(
          value.value.hexValue.begin(),
          value.value.hexValue.end(),
          conv.begin());
      target.value()->set_rawValue(conv);
    } break;
    case rackmon::RegisterValueType::INTEGER:
      target.type() = RegisterValueType::INTEGER;
      target.value()->set_intValue(value.value.intValue);
      break;
    case rackmon::RegisterValueType::STRING:
      target.type() = RegisterValueType::STRING;
      target.value()->set_strValue(value.value.strValue);
      break;
    case rackmon::RegisterValueType::FLOAT:
      target.type() = RegisterValueType::FLOAT;
      target.value()->set_floatValue(value.value.floatValue);
      break;
    case rackmon::RegisterValueType::FLAGS: {
      target.type() = RegisterValueType::FLAGS;
      std::vector<FlagType> flags;
      for (const auto& [flagVal, flagName, flagPos] : value.value.flagsValue) {
        FlagType flag;
        flag.name() = flagName;
        flag.value() = flagVal;
        flag.bitOffset() = flagPos;
        flags.emplace_back(flag);
      }
      target.value()->set_flagsValue(flags);
    } break;
    default:
      throw std::runtime_error("Unsupported Value");
  }
  return target;
}

ModbusRegisterStore ThriftHandler::transformRegisterStoreValue(
    const rackmon::RegisterStoreValue& source) {
  ModbusRegisterStore target;

  target.regAddress() = source.regAddr;
  target.name() = source.name;
  for (const auto& reg : source.history) {
    target.history()->emplace_back(transformRegisterValue(reg));
  }
  return target;
}

RackmonMonitorData ThriftHandler::transformModbusDeviceValueData(
    const rackmon::ModbusDeviceValueData& source) {
  RackmonMonitorData data;
  data.devInfo() = transformModbusDeviceInfo(source);
  for (const auto& reg : source.registerList) {
    data.regList()->emplace_back(transformRegisterStoreValue(reg));
  }
  return data;
}

ThriftHandler::ThriftHandler() {
  rackmond_.loadInterface(nlohmann::json::parse(getInterfaceConfig()));
  const std::vector<std::string> regMaps = getRegisterMapConfig();
  for (const auto& regmap : regMaps) {
    rackmond_.loadRegisterMap(nlohmann::json::parse(regmap));
  }
  rackmond_.start();
}

void ThriftHandler::listModbusDevices(std::vector<ModbusDeviceInfo>& devices) {
  std::vector<rackmon::ModbusDeviceInfo> info = rackmond_.listDevices();
  for (auto& dev : info) {
    devices.emplace_back(transformModbusDeviceInfo(dev));
  }
}

void ThriftHandler::getMonitorData(std::vector<RackmonMonitorData>& data) {
  std::vector<rackmon::ModbusDeviceValueData> indata;
  rackmond_.getValueData(indata);
  for (auto& dev : indata) {
    data.emplace_back(transformModbusDeviceValueData(dev));
  }
}

void ThriftHandler::readHoldingRegisters(
    ReadWordRegistersResponse& response,
    std::unique_ptr<ReadWordRegistersRequest> request) {
  try {
    std::vector<uint16_t> regs(request->get_numRegisters());
    int32_t* timeout = request->get_timeout();
    uint8_t devAddr = request->get_devAddress();
    uint16_t regAddr = request->get_regAddress();
    if (timeout) {
      rackmon::ModbusTime tmo(*timeout);
      rackmond_.readHoldingRegisters(devAddr, regAddr, regs, tmo);
    } else {
      rackmond_.readHoldingRegisters(devAddr, regAddr, regs);
    }
    response.status() = RackmonStatusCode::SUCCESS;
    response.regValues()->resize(regs.size());
    std::copy(regs.begin(), regs.end(), response.regValues()->begin());
  } catch (rackmon::TimeoutException& ex) {
    response.status() = RackmonStatusCode::ERR_TIMEOUT;
  } catch (rackmon::CRCError& ex) {
    response.status() = RackmonStatusCode::ERR_BAD_CRC;
  } catch (rackmon::BadResponseError& ex) {
    response.status() = RackmonStatusCode::ERR_IO_FAILURE;
  } catch (std::out_of_range& ex) {
    // Unknown device or not in active status.
    response.status() = RackmonStatusCode::ERR_INVALID_ARGS;
  } catch (std::exception& ex) {
    response.status() = RackmonStatusCode::ERR_IO_FAILURE;
  }
}

RackmonStatusCode ThriftHandler::writeSingleRegister(
    std::unique_ptr<WriteSingleRegisterRequest> request) {
  try {
    int32_t* timeout = request->get_timeout();
    uint8_t devAddr = request->get_devAddress();
    uint16_t regAddr = request->get_regAddress();
    uint16_t regValue = request->get_regValue();
    if (timeout) {
      rackmon::ModbusTime tmo(*timeout);
      rackmond_.writeSingleRegister(devAddr, regAddr, regValue, tmo);
    } else {
      rackmond_.writeSingleRegister(devAddr, regAddr, regValue);
    }
    return RackmonStatusCode::SUCCESS;
  } catch (rackmon::TimeoutException& ex) {
    return RackmonStatusCode::ERR_TIMEOUT;
  } catch (rackmon::CRCError& ex) {
    return RackmonStatusCode::ERR_BAD_CRC;
  } catch (rackmon::BadResponseError& ex) {
    return RackmonStatusCode::ERR_IO_FAILURE;
  } catch (std::out_of_range& ex) {
    // Unknown device or not in active status.
    return RackmonStatusCode::ERR_INVALID_ARGS;
  } catch (std::exception& ex) {
    return RackmonStatusCode::ERR_IO_FAILURE;
  }
}

RackmonStatusCode ThriftHandler::presetMultipleRegisters(
    std::unique_ptr<PresetMultipleRegistersRequest> request) {
  try {
    int32_t* timeout = request->get_timeout();
    uint8_t devAddr = request->get_devAddress();
    uint16_t regAddr = request->get_regAddress();
    std::vector<uint16_t> regValues;
    std::copy(
        request->get_regValue().begin(),
        request->get_regValue().end(),
        std::back_inserter(regValues));
    if (timeout) {
      rackmon::ModbusTime tmo(*timeout);
      rackmond_.writeMultipleRegisters(devAddr, regAddr, regValues, tmo);
    } else {
      rackmond_.writeMultipleRegisters(devAddr, regAddr, regValues);
    }
    return RackmonStatusCode::SUCCESS;
  } catch (rackmon::TimeoutException& ex) {
    return RackmonStatusCode::ERR_TIMEOUT;
  } catch (rackmon::CRCError& ex) {
    return RackmonStatusCode::ERR_BAD_CRC;
  } catch (rackmon::BadResponseError& ex) {
    return RackmonStatusCode::ERR_IO_FAILURE;
  } catch (std::out_of_range& ex) {
    // Unknown device or not in active status.
    return RackmonStatusCode::ERR_INVALID_ARGS;
  } catch (std::exception& ex) {
    return RackmonStatusCode::ERR_IO_FAILURE;
  }
}

void ThriftHandler::readFileRecord(
    ReadFileRecordResponse& response,
    std::unique_ptr<ReadFileRecordRequest> request) {
  try {
    int32_t* timeout = request->get_timeout();
    uint8_t devAddr = request->get_devAddress();
    std::vector<rackmon::FileRecord> records;
    for (const auto& rec : request->get_records()) {
      records.emplace_back(
          (uint16_t)rec.get_fileNum(),
          (uint16_t)rec.get_recordNum(),
          (size_t)rec.get_dataSize());
    }
    if (timeout) {
      rackmon::ModbusTime tmo(*timeout);
      rackmond_.readFileRecord(devAddr, records, tmo);
    } else {
      rackmond_.readFileRecord(devAddr, records);
    }
    response.status() = RackmonStatusCode::SUCCESS;
    auto transformFileRecord = [](const auto& rec) {
      FileRecord ret;
      ret.fileNum() = rec.fileNum;
      ret.recordNum() = rec.recordNum;
      std::vector<int32_t> data(rec.data.size());
      std::copy(rec.data.begin(), rec.data.end(), data.begin());
      ret.data() = data;
      return ret;
    };
    for (const auto& rec : records) {
      response.data()->emplace_back(transformFileRecord(rec));
    }
  } catch (rackmon::TimeoutException& ex) {
    response.status() = RackmonStatusCode::ERR_TIMEOUT;
  } catch (rackmon::CRCError& ex) {
    response.status() = RackmonStatusCode::ERR_BAD_CRC;
  } catch (rackmon::BadResponseError& ex) {
    response.status() = RackmonStatusCode::ERR_IO_FAILURE;
  } catch (std::out_of_range& ex) {
    // Unknown device or not in active status.
    response.status() = RackmonStatusCode::ERR_INVALID_ARGS;
  } catch (std::exception& ex) {
    response.status() = RackmonStatusCode::ERR_IO_FAILURE;
  }
}

RackmonStatusCode ThriftHandler::controlRackmond(
    RackmonControlRequest request) {
  try {
    switch (request) {
      case RackmonControlRequest::PAUSE_RACKMOND:
        rackmond_.stop();
        break;
      case RackmonControlRequest::RESUME_RACKMOND:
        rackmond_.start();
        break;
      default:
        return RackmonStatusCode::ERR_INVALID_ARGS;
    }
  } catch (std::exception& ex) {
    return RackmonStatusCode::ERR_IO_FAILURE;
  }
  return RackmonStatusCode::SUCCESS;
}
} // namespace rackmonsvc
