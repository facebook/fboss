// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <map>
#include <memory>
#include <string>

#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
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

} // namespace facebook::fboss::hal_test
