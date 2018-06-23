/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmRtag7Module.h"

namespace facebook {
namespace fboss {

/*
 * The below constants use the following nomenclature:
 * - "Field0" and "Field1" identify module A and module B, respectively
 * - "Config" and "Config1" identify the first and second 16-bit output of the
 *    hash computation block, respectively
 */
const BcmRtag7Module::ModuleControl BcmRtag7Module::kModuleAControl() {
  static const ModuleControl moduleAControl = {
      'A',
      PreprocessingControl(opennslSwitchHashField0PreProcessEnable),
      SeedControl(opennslSwitchHashSeed0),
      0, // BCM_TH_HASH_BIN_A0
      6, // BCM_TH_HASH_BIN_A1
      IPv4NonTcpUdpFieldSelectionControl(opennslSwitchHashIP4Field0),
      IPv6NonTcpUdpFieldSelectionControl(opennslSwitchHashIP6Field0),
      IPv4TcpUdpFieldSelectionControl(opennslSwitchHashIP4TcpUdpField0),
      IPv6TcpUdpFieldSelectionControl(opennslSwitchHashIP6TcpUdpField0),
      IPv4TcpUdpPortsEqualFieldSelectionControl(
          opennslSwitchHashIP4TcpUdpPortsEqualField0),
      IPv6TcpUdpPortsEqualFieldSelectionControl(
          opennslSwitchHashIP6TcpUdpPortsEqualField0),
      FirstOutputFunctionControl(opennslSwitchHashField0Config),
      SecondOutputFunctionControl(opennslSwitchHashField0Config1)};
  return moduleAControl;
}

const BcmRtag7Module::ModuleControl BcmRtag7Module::kModuleBControl() {
  static const ModuleControl moduleBControl = {
      'B',
      PreprocessingControl(opennslSwitchHashField1PreProcessEnable),
      SeedControl(opennslSwitchHashSeed1),
      1, // BCM_TH_HASH_BIN_B0
      7, // BCM_TH_HASH_BIN_B1
      IPv4NonTcpUdpFieldSelectionControl(opennslSwitchHashIP4Field1),
      IPv6NonTcpUdpFieldSelectionControl(opennslSwitchHashIP6Field1),
      IPv4TcpUdpFieldSelectionControl(opennslSwitchHashIP4TcpUdpField1),
      IPv6TcpUdpFieldSelectionControl(opennslSwitchHashIP6TcpUdpField1),
      IPv4TcpUdpPortsEqualFieldSelectionControl(
          opennslSwitchHashIP4TcpUdpPortsEqualField1),
      IPv6TcpUdpPortsEqualFieldSelectionControl(
          opennslSwitchHashIP6TcpUdpPortsEqualField1),
      FirstOutputFunctionControl(opennslSwitchHashField1Config),
      SecondOutputFunctionControl(opennslSwitchHashField1Config1)};
  return moduleBControl;
}

BcmRtag7Module::BcmRtag7Module(
    BcmRtag7Module::ModuleControl moduleControl,
    opennsl_switch_control_t offset,
    const BcmSwitch* hw)
    : moduleControl_(moduleControl), offset_(offset), hw_(hw) {}

BcmRtag7Module::~BcmRtag7Module() {
  // There's no work necessary to clean up an RTAG7 module as on next use it
  // it will be re-programmed
}

} // namespace fboss
} // namespace facebook
