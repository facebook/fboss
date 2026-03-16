// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/module/Transceiver.h"
#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/test/hal_test/BspTransceiverImpl.h"
#include "fboss/qsfp_service/test/hal_test/gen-cpp2/hal_test_config_types.h"

namespace facebook::fboss::hal_test {

// Build a BspTransceiverMapping for a transceiver entry from config
// attributes or hardcoded devmap defaults.
BspTransceiverMapping buildBspTransceiverMapping(
    const HalTestTransceiverEntry& entry);

// Create a BspTransceiverImpl for a transceiver.
std::unique_ptr<BspTransceiverImpl> createBspTransceiverImpl(
    const HalTestTransceiverEntry& entry);

// Create a QsfpModule + its BspTransceiverImpl.
struct HalTestModule {
  std::unique_ptr<QsfpModule> module;
  std::unique_ptr<BspTransceiverImpl> impl;
};

HalTestModule createQsfpModule(const HalTestTransceiverEntry& entry);

// Create QsfpModules for all transceivers in the config.
std::map<int, HalTestModule> createAllQsfpModules(const HalTestConfig& config);

// Load HalTestConfig from a JSON file.
HalTestConfig loadHalTestConfig(const std::string& configPath);

ProgramTransceiverState createProgramTransceiverState(TcvrOperationalMode mode);

// Returns the expected per-lane MediaInterfaceCodes for a given operational
// mode.
std::vector<MediaInterfaceCode> getExpectedMediaInterfaceCodes(
    TcvrOperationalMode mode);

// Returns the effective media interface configs — user-provided if set,
// otherwise falls back to the thrift const default.
const std::map<MediaInterfaceCode, HalTestMediaInterfaceConfig>&
getMediaInterfaceConfigs(const HalTestConfig& config);

// Collect all speed-change transitions from a media-interface config map.
std::vector<std::pair<TcvrOperationalMode, TcvrOperationalMode>>
getAllSpeedChangeTransitions(
    const std::map<MediaInterfaceCode, HalTestMediaInterfaceConfig>& configs);

// Check if a module's media interface code has a given transition in its
// config.
bool isSpeedChangeSupportedForModule(
    QsfpModule* module,
    const HalTestConfig& config,
    TcvrOperationalMode from,
    TcvrOperationalMode to);

// Upgrade firmware on a module to the specified versions.
// Returns true if an upgrade was performed.
bool upgradeFirmware(QsfpModule* module, const cfg::Firmware& desiredFw);

// Apply firmware upgrades specified in startup configs.
// Returns the number of modules that were upgraded.
int applyStartupFirmwareUpgrades(
    const HalTestConfig& config,
    std::map<int, HalTestModule>& modules);

} // namespace facebook::fboss::hal_test
