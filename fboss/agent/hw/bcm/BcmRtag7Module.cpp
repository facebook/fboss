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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"

#include <folly/logging/xlog.h>

#include <algorithm>

namespace {
const bool kEnable = true;
} // namespace

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

bool BcmRtag7Module::fieldControlProgrammed_ = false;

BcmRtag7Module::BcmRtag7Module(
    BcmRtag7Module::ModuleControl moduleControl,
    opennsl_switch_control_t offset,
    const BcmSwitch* hw)
    : moduleControl_(moduleControl), offset_(offset), hw_(hw) {}

BcmRtag7Module::~BcmRtag7Module() {
  // There's no work necessary to clean up an RTAG7 module as on next use it
  // it will be re-programmed
}

void BcmRtag7Module::programMacroFlowHashing(bool enable) {
  auto rv = setUnitControl(opennslSwitchEcmpMacroFlowHashEnable, enable);
  bcmCheckError(rv, "failed to ", enable, "program macro flow hashing");
}

void BcmRtag7Module::programPreprocessing(bool enable) {
  auto rv = setUnitControl(moduleControl_.enablePreprocessing, enable);
  bcmCheckError(
      rv, "failed to enable pre-processing on module ", moduleControl_.module);
}

void BcmRtag7Module::programAlgorithm(cfg::HashingAlgorithm algorithm) {
  int rv;
  int arg;

  if (algorithm == cfg::HashingAlgorithm::CRC16_CCITT) {
    arg = OPENNSL_HASH_FIELD_CONFIG_CRC16CCITT;
  } else {
    throw FbossError("Unrecognized HashingAlgorithm");
  }

  // Each block calculates two 16-bit hashes using two different functions. We
  // currently do not leverage the second hash value so we set its function to
  // be that of the first hash value (in effect wasting it).
  rv = setUnitControl(moduleControl_.hashFunction1, arg);
  bcmCheckError(
      rv,
      "failed to configure function ",
      algorithm,
      " on ",
      moduleControl_.module,
      "0");

  rv = setUnitControl(moduleControl_.hashFunction2, arg);
  bcmCheckError(
      rv,
      "failed to configure function ",
      algorithm,
      " on ",
      moduleControl_.module,
      "1");
}

void BcmRtag7Module::programOutputSelection() {
  // Because both outputs are programmed with the same hashing algorithm (see
  // BcmRtag7Modlue::programAlgorithm), it does not matter which output we
  // use. For simplicity, the first output is always selected.
  int rv = setUnitControl(offset_, moduleControl_.selectFirstOutput);
  bcmCheckError(rv, "failed to select first output on ", moduleControl_.module);
}

void BcmRtag7Module::programFieldSelection(
    LoadBalancer::IPv4FieldsRange v4FieldsRange,
    LoadBalancer::IPv6FieldsRange v6FieldsRange,
    LoadBalancer::TransportFieldsRange transportFieldsRange) {
  programIPv4FieldSelection(v4FieldsRange, transportFieldsRange);
  programIPv6FieldSelection(v6FieldsRange, transportFieldsRange);
}

void BcmRtag7Module::programIPv4FieldSelection(
    LoadBalancer::IPv4FieldsRange v4FieldsRange,
    LoadBalancer::TransportFieldsRange transportFieldsRange) {
  int arg = computeIPv4Subfields(v4FieldsRange);
  arg |= computeTransportSubfields(transportFieldsRange);

  // TODO(samank): why do we set OPENNSL_HASH_FIELD_{SRC,DST}L4 here...
  int rv = setUnitControl(moduleControl_.ipv4NonTcpUdpFieldSelection, arg);
  bcmCheckError(rv, "failed to config field selection");

  rv = setUnitControl(moduleControl_.ipv4TcpUdpPortsUnequalFieldSelection, arg);
  bcmCheckError(rv, "failed to config field selection");

  // We make no distinction between a TCP/UDP segment whose source port does not
  // equal its destination port (the previous case) and one whose source port
  // is equal to its destination port (this case)
  rv = setUnitControl(moduleControl_.ipv4TcpUdpPortsEqualFieldSelection, arg);
  bcmCheckError(rv, "failed to config field selection");
}

void BcmRtag7Module::programIPv6FieldSelection(
    LoadBalancer::IPv6FieldsRange v6FieldsRange,
    LoadBalancer::TransportFieldsRange transportFieldsRange) {
  auto it = std::find(
      v6FieldsRange.begin(),
      v6FieldsRange.end(),
      LoadBalancer::IPv6Field::FLOW_LABEL);
  if (it != v6FieldsRange.end()) {
    enableFlowLabelSelection();
  }

  int arg = computeIPv6Subfields(v6FieldsRange);
  arg |= computeTransportSubfields(transportFieldsRange);

  // TODO(samank): why do we set OPENNSL_HASH_FIELD_{SRC,DST}L4 here...
  int rv = setUnitControl(moduleControl_.ipv6NonTcpUdpFieldSelection, arg);
  bcmCheckError(rv, "failed to config field selection");

  rv = setUnitControl(moduleControl_.ipv6TcpUdpPortsUnequalFieldSelection, arg);
  bcmCheckError(rv, "failed to config field selection");

  // We make no distinction between a TCP/UDP segment whose source port does not
  // equal its destination port (the previous case) and one whose source port
  // is equal to its destination port (this case)
  rv = setUnitControl(moduleControl_.ipv6TcpUdpPortsEqualFieldSelection, arg);
  bcmCheckError(rv, "failed to config field selection");
}

void BcmRtag7Module::programSeed(uint32_t seed) {
  auto rv = setUnitControl(moduleControl_.setSeed, static_cast<int>(seed));
  bcmCheckError(rv, "failed to set seed on module ", moduleControl_.module);
}

void BcmRtag7Module::enableRtag7(LoadBalancerID loadBalancerID) {
  int rv = 0;

  switch (loadBalancerID) {
    case LoadBalancerID::ECMP:
      rv = setUnitControl(
          opennslSwitchHashControl, OPENNSL_HASH_CONTROL_ECMP_ENHANCE);
      bcmCheckError(rv, "failed to enable RTAG7 for ECMP");
      break;
    case LoadBalancerID::AGGREGATE_PORT:
      // RTAG7 for trunks is enabled via bcm_trunk_info_t.psc
      break;
  }
}

void BcmRtag7Module::init(const std::shared_ptr<LoadBalancer>& loadBalancer) {
  // By unconditonally enabling preprocessing, we are keeping with old behavior
  programPreprocessing(kEnable);

  programSeed(loadBalancer->getSeed());

  programFieldSelection(
      loadBalancer->getIPv4Fields(),
      loadBalancer->getIPv6Fields(),
      loadBalancer->getTransportFields());

  programAlgorithm(loadBalancer->getAlgorithm());

  programOutputSelection();

  if (!fieldControlProgrammed_) {
    programFieldControl();
    fieldControlProgrammed_ = true;
  }

  // Macro-flow hashing was originally enabled to work around a bug on the
  // Trident+ chipset (see D1223184 for details). Because the bug may also
  // affect Trident2 and Tomahawk chipsets, we unconditionally enable it.
  // TODO(samank): figure out if bug affects Trident2 and Tomahawk chipsets
  programMacroFlowHashing(kEnable);

  enableRtag7(loadBalancer->getID());
}

void BcmRtag7Module::program(
    const std::shared_ptr<LoadBalancer>& oldLoadBalancer,
    const std::shared_ptr<LoadBalancer>& newLoadBalancer) {
  if (oldLoadBalancer->getSeed() != newLoadBalancer->getSeed()) {
    programSeed(newLoadBalancer->getSeed());
  }

  bool sameIPv4Fields = std::equal(
      oldLoadBalancer->getIPv4Fields().begin(),
      oldLoadBalancer->getIPv4Fields().end(),
      newLoadBalancer->getIPv4Fields().begin(),
      newLoadBalancer->getIPv4Fields().end());
  bool sameIPv6Fields = std::equal(
      oldLoadBalancer->getIPv6Fields().begin(),
      oldLoadBalancer->getIPv6Fields().end(),
      newLoadBalancer->getIPv6Fields().begin(),
      newLoadBalancer->getIPv6Fields().end());
  bool sameTransportFields = std::equal(
      oldLoadBalancer->getTransportFields().begin(),
      oldLoadBalancer->getTransportFields().end(),
      newLoadBalancer->getTransportFields().begin(),
      newLoadBalancer->getTransportFields().end());

  if (!sameIPv4Fields || !sameTransportFields) {
    programIPv4FieldSelection(
        newLoadBalancer->getIPv4Fields(),
        newLoadBalancer->getTransportFields());
  }

  if (!sameIPv6Fields || !sameTransportFields) {
    programIPv6FieldSelection(
        newLoadBalancer->getIPv6Fields(),
        newLoadBalancer->getTransportFields());
  }

  if (oldLoadBalancer->getAlgorithm() != newLoadBalancer->getAlgorithm()) {
    programAlgorithm(newLoadBalancer->getAlgorithm());
  }

  // This is unnecessary because init() is always invoked before program()
  // and will have already picked off the first output.
  programOutputSelection();
}

int BcmRtag7Module::computeIPv4Subfields(
    LoadBalancer::IPv4FieldsRange v4FieldsRange) const {
  int subfields = 0;

  for (const auto& v4Field : v4FieldsRange) {
    switch (v4Field) {
      case LoadBalancer::IPv4Field::SOURCE_ADDRESS:
        subfields |=
            (OPENNSL_HASH_FIELD_IP4SRC_LO | OPENNSL_HASH_FIELD_IP4SRC_HI);
        break;
      case LoadBalancer::IPv4Field::DESTINATION_ADDRESS:
        subfields |=
            (OPENNSL_HASH_FIELD_IP4DST_LO | OPENNSL_HASH_FIELD_IP4DST_HI);
        break;
    }
  }

  return subfields;
}

int BcmRtag7Module::computeIPv6Subfields(
    LoadBalancer::IPv6FieldsRange v6FieldsRange) const {
  int subfields = 0;

  for (const auto& v6Field : v6FieldsRange) {
    switch (v6Field) {
      case LoadBalancer::IPv6Field::SOURCE_ADDRESS:
        subfields |=
            (OPENNSL_HASH_FIELD_IP6SRC_LO | OPENNSL_HASH_FIELD_IP6SRC_HI);
        break;
      case LoadBalancer::IPv6Field::DESTINATION_ADDRESS:
        subfields |=
            (OPENNSL_HASH_FIELD_IP6DST_LO | OPENNSL_HASH_FIELD_IP6DST_HI);
        break;
      case LoadBalancer::IPv6Field::FLOW_LABEL:
        subfields |= getFlowLabelSubfields();
        break;
    }
  }

  return subfields;
}

int BcmRtag7Module::computeTransportSubfields(
    LoadBalancer::TransportFieldsRange transportFieldsRange) const {
  int subfields = 0;

  for (const auto& transportField : transportFieldsRange) {
    switch (transportField) {
      case LoadBalancer::TransportField::SOURCE_PORT:
        subfields |= OPENNSL_HASH_FIELD_SRCL4;
        break;
      case LoadBalancer::TransportField::DESTINATION_PORT:
        subfields |= OPENNSL_HASH_FIELD_DSTL4;
        break;
    }
  }

  return subfields;
}

void BcmRtag7Module::programFieldControl() {
  int kDefault = 0;

  // This SDK call controls two types of behavior related to field selection:
  // 1) If shallow-parsing should be enabled for an Ethertype which could
  //    otherwise be deep-parsed.
  // 2) If fields from the inner or outer header should be used in the case
  //    of tunneling.
  auto rv = setUnitControl(opennslSwitchHashSelectControl, kDefault);
  bcmCheckError(rv, "failed to program RTAG7 field control");
}

int BcmRtag7Module::getUnitControl(int unit, opennsl_switch_control_t type) {
  int val = 0;

  int rv = opennsl_switch_control_get(unit, type, &val);
  bcmCheckError(rv, "failed to retrieve value for ", type);

  return val;
}

template <typename ModuleControlType>
int BcmRtag7Module::setUnitControl(ModuleControlType controlType, int arg) {
  auto wbCache = hw_->getWarmBootCache();

  if (wbCache->unitControlMatches(
          moduleControl_.module, controlType, arg)) {
    XLOG(DBG2) << "Skipping assigning " << arg << " to " << controlType;

    wbCache->programmed(moduleControl_.module, controlType);

    return OPENNSL_E_NONE;
  }

  return opennsl_switch_control_set(hw_->getUnit(), controlType, arg);
}

BcmRtag7Module::ModuleState BcmRtag7Module::retrieveRtag7ModuleState(
    int unit,
    ModuleControl control) {
  ModuleState state;

  // TODO(samank): the names in ModuleControl read a bit awkwardly now that
  // they're not only passed to setUnitControl. I will change them to be read/
  // write neutral eg., s/enablePreprocessing/preprocessing.
  state[control.enablePreprocessing] =
      BcmRtag7Module::getUnitControl(unit, control.enablePreprocessing);
  state[control.setSeed] =
      BcmRtag7Module::getUnitControl(unit, control.setSeed);
  state[control.ipv4NonTcpUdpFieldSelection] =
      BcmRtag7Module::getUnitControl(unit, control.ipv4NonTcpUdpFieldSelection);
  state[control.ipv4TcpUdpPortsUnequalFieldSelection] =
      BcmRtag7Module::getUnitControl(
          unit, control.ipv4TcpUdpPortsUnequalFieldSelection);
  state[control.ipv4TcpUdpPortsEqualFieldSelection] =
      BcmRtag7Module::getUnitControl(
          unit, control.ipv4TcpUdpPortsEqualFieldSelection);
  state[control.ipv6NonTcpUdpFieldSelection] =
      BcmRtag7Module::getUnitControl(unit, control.ipv6NonTcpUdpFieldSelection);
  state[control.ipv6TcpUdpPortsUnequalFieldSelection] =
      BcmRtag7Module::getUnitControl(
          unit, control.ipv6TcpUdpPortsUnequalFieldSelection);
  state[control.ipv6TcpUdpPortsEqualFieldSelection] =
      BcmRtag7Module::getUnitControl(
          unit, control.ipv6TcpUdpPortsEqualFieldSelection);
  state[control.hashFunction1] =
      BcmRtag7Module::getUnitControl(unit, control.hashFunction1);
  state[control.hashFunction2] =
      BcmRtag7Module::getUnitControl(unit, control.hashFunction2);

  return state;
}

} // namespace fboss
} // namespace facebook
