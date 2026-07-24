// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_properties_types.h"
#include "fboss/qsfp_service/module/Transceiver.h"
#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/test/hal_test/BspTransceiverImpl.h"
#include "fboss/qsfp_service/test/hal_test/gen-cpp2/hal_test_config_types.h"

namespace facebook::fboss::hal_test {

// Create a BspTransceiverImpl for a transceiver. Uses the entry's BSP path
// overrides if all three are set (non-FBOSS mode); otherwise looks the
// transceiver up in the platform's BspPlatformMapping. Throws FbossError if the
// entry has partial overrides, if the non-override path is taken with a null
// bspMapping, or if the transceiver is absent from bspMapping.
std::unique_ptr<BspTransceiverImpl> createBspTransceiverImpl(
    const HalTestTransceiverEntry& entry,
    const BspPlatformMapping* bspMapping);

// Create a QsfpModule + its BspTransceiverImpl.
struct HalTestModule {
  std::unique_ptr<QsfpModule> module;
  std::unique_ptr<BspTransceiverImpl> impl;
};

HalTestModule createQsfpModule(
    const HalTestTransceiverEntry& entry,
    const BspPlatformMapping* bspMapping);

// Create QsfpModules for all transceivers in the config.
std::map<int, HalTestModule> createAllQsfpModules(const HalTestConfig& config);

// Load HalTestConfig from a JSON file.
HalTestConfig loadHalTestConfig(const std::string& configPath);

// Build a ProgramTransceiverState from a SpeedCombination's port list.
ProgramTransceiverState createProgramTransceiverState(
    const SpeedCombination& combo);

// Build expected per-lane MediaInterfaceCodes from a SpeedCombination.
std::vector<MediaInterfaceCode> getExpectedMediaInterfaceCodes(
    const std::string& comboDescription,
    const SpeedCombination& combo);

// Collect all speed combination descriptions from TransceiverPropertiesManager.
std::vector<std::string> getAllSpeedCombinationDescriptions();

// Collect all speed-change transitions from TransceiverPropertiesManager.
std::vector<std::pair<std::string, std::string>> getAllSpeedChangeTransitions();

// Detect the module and bring it out of the post-reset low-power state, waiting
// until it reports ready so subsequent I2C reads / refresh succeed. A module
// fresh out of a hard reset comes up in low power and not yet ready, so paged
// reads fail until this runs. No-op (beyond detectPresence) for non-CMIS
// modules. Best effort: transient I2C errors are logged, not thrown.
void ensureModuleReady(QsfpModule* module);

// Upgrade firmware on a module to the specified versions.
// Returns true if an upgrade was performed.
bool upgradeFirmware(QsfpModule* module, const cfg::Firmware& desiredFw);

// Apply firmware upgrades specified in startup configs.
// Returns the number of modules that were upgraded.
int applyStartupFirmwareUpgrades(
    const HalTestConfig& config,
    std::map<int, HalTestModule>& modules);

} // namespace facebook::fboss::hal_test
