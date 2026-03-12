// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/test/hal_test/HalTestUtils.h"

#include <folly/Conv.h>
#include <folly/FileUtil.h>
#include <folly/Format.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/FbossError.h"

namespace facebook::fboss::hal_test {

BspTransceiverMapping buildBspTransceiverMapping(
    const HalTestTransceiverEntry& entry) {
  int id = *entry.id();

  BspTransceiverMapping mapping;
  mapping.tcvrId() = id;

  // IO
  BspTransceiverIOControllerInfo io;
  io.controllerId() = folly::to<std::string>(id);
  io.type() = TransceiverIOType::I2C;
  if (entry.i2cDevicePath().has_value()) {
    io.devicePath() = *entry.i2cDevicePath();
  } else {
    io.devicePath() = fmt::format("/run/devmap/xcvrs/xcvr_io_{}", id);
  }
  mapping.io() = io;

  // Access control
  BspTransceiverAccessControllerInfo ac;
  ac.controllerId() = folly::to<std::string>(id);
  ac.type() = ResetAndPresenceAccessType::CPLD;

  BspPresencePinInfo presence;
  if (entry.presentPath().has_value()) {
    presence.sysfsPath() = *entry.presentPath();
  } else {
    presence.sysfsPath() =
        fmt::format("/run/devmap/xcvrs/xcvr_ctrl_{}/xcvr_present_{}", id, id);
  }
  presence.mask() = 1;
  presence.presentHoldHi() = 0;
  ac.presence() = presence;

  BspResetPinInfo reset;
  if (entry.resetPath().has_value()) {
    reset.sysfsPath() = *entry.resetPath();
  } else {
    reset.sysfsPath() =
        fmt::format("/run/devmap/xcvrs/xcvr_ctrl_{}/xcvr_reset_{}", id, id);
  }
  reset.mask() = 1;
  reset.resetHoldHi() = 0;
  ac.reset() = reset;

  mapping.accessControl() = ac;
  return mapping;
}

std::unique_ptr<BspTransceiverImpl> createBspTransceiverImpl(
    const HalTestTransceiverEntry& entry) {
  auto mapping = buildBspTransceiverMapping(entry);
  return std::make_unique<BspTransceiverImpl>(
      *entry.id(), *entry.name(), std::move(mapping));
}

HalTestModule createQsfpModule(const HalTestTransceiverEntry& entry) {
  HalTestModule result;
  result.impl = createBspTransceiverImpl(entry);

  std::set<std::string> portNames;
  portNames.insert(*entry.name());

  auto cfg = std::make_shared<const TransceiverConfig>(TransceiverOverrides{});

  result.module = std::make_unique<CmisModule>(
      std::move(portNames),
      result.impl.get(),
      std::move(cfg),
      false, // supportRemediate
      *entry.name());

  return result;
}

std::map<int, HalTestModule> createAllQsfpModules(const HalTestConfig& config) {
  std::map<int, HalTestModule> modules;
  for (const auto& entry : *config.transceivers()) {
    int id = *entry.id();
    modules.emplace(id, createQsfpModule(entry));
  }
  return modules;
}

HalTestConfig loadHalTestConfig(const std::string& configPath) {
  std::string contents;
  if (!folly::readFile(configPath.c_str(), contents)) {
    throw FbossError("Failed to read HAL test config file: ", configPath);
  }
  return apache::thrift::SimpleJSONSerializer::deserialize<HalTestConfig>(
      contents);
}

} // namespace facebook::fboss::hal_test
