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
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include <folly/lang/Assume.h>
#include <folly/logging/xlog.h>

#include <algorithm>

extern "C" {
#include <bcm/switch.h>
}

namespace {
const bool kEnable = true;
} // namespace

namespace facebook::fboss {

/*
 * The below constants use the following nomenclature:
 * - "Field0" and "Field1" identify module A and module B, respectively
 * - "Config" and "Config1" identify the first and second 16-bit output of the
 *    hash computation block, respectively
 */
const BcmRtag7Module::ModuleControl BcmRtag7Module::kModuleAControl() {
  static const ModuleControl moduleAControl = {
      'A',
      PreprocessingControl(bcmSwitchHashField0PreProcessEnable),
      SeedControl(bcmSwitchHashSeed0),
      {0, 15}, // A_0
      {68, 83}, // A_1
      IPv4NonTcpUdpFieldSelectionControl(bcmSwitchHashIP4Field0),
      IPv6NonTcpUdpFieldSelectionControl(bcmSwitchHashIP6Field0),
      IPv4TcpUdpFieldSelectionControl(bcmSwitchHashIP4TcpUdpField0),
      IPv6TcpUdpFieldSelectionControl(bcmSwitchHashIP6TcpUdpField0),
      IPv4TcpUdpPortsEqualFieldSelectionControl(
          bcmSwitchHashIP4TcpUdpPortsEqualField0),
      IPv6TcpUdpPortsEqualFieldSelectionControl(
          bcmSwitchHashIP6TcpUdpPortsEqualField0),
      getTerminatedMPLSFieldSelectionControl('A'),
      getNonTerminatedMPLSFieldSelectionControl('A'),
      FirstOutputFunctionControl(bcmSwitchHashField0Config),
      SecondOutputFunctionControl(bcmSwitchHashField0Config1)};
  return moduleAControl;
}

const BcmRtag7Module::ModuleControl BcmRtag7Module::kModuleBControl() {
  static const ModuleControl moduleBControl = {
      'B',
      PreprocessingControl(bcmSwitchHashField1PreProcessEnable),
      SeedControl(bcmSwitchHashSeed1),
      {16, 31}, // B_0
      {84, 99}, // B_1
      IPv4NonTcpUdpFieldSelectionControl(bcmSwitchHashIP4Field1),
      IPv6NonTcpUdpFieldSelectionControl(bcmSwitchHashIP6Field1),
      IPv4TcpUdpFieldSelectionControl(bcmSwitchHashIP4TcpUdpField1),
      IPv6TcpUdpFieldSelectionControl(bcmSwitchHashIP6TcpUdpField1),
      IPv4TcpUdpPortsEqualFieldSelectionControl(
          bcmSwitchHashIP4TcpUdpPortsEqualField1),
      IPv6TcpUdpPortsEqualFieldSelectionControl(
          bcmSwitchHashIP6TcpUdpPortsEqualField1),
      getTerminatedMPLSFieldSelectionControl('B'),
      getNonTerminatedMPLSFieldSelectionControl('B'),
      FirstOutputFunctionControl(bcmSwitchHashField1Config),
      SecondOutputFunctionControl(bcmSwitchHashField1Config1)};
  return moduleBControl;
}

bool BcmRtag7Module::fieldControlProgrammed_ = false;

BcmRtag7Module::BcmRtag7Module(
    BcmRtag7Module::ModuleControl moduleControl,
    BcmRtag7Module::OutputSelectionControl outputControl,
    const BcmSwitch* hw)
    : moduleControl_(moduleControl), outputControl_(outputControl), hw_(hw) {}

BcmRtag7Module::~BcmRtag7Module() {
  // There's no work necessary to clean up an RTAG7 module as on next use it
  // it will be re-programmed
}

void BcmRtag7Module::programPreprocessing(bool enable) {
  auto rv = setUnitControl(moduleControl_.enablePreprocessing, enable);
  bcmCheckError(
      rv, "failed to enable pre-processing on module ", moduleControl_.module);
}

void BcmRtag7Module::programAlgorithm(cfg::HashingAlgorithm algorithm) {
  int rv;

  int arg = getBcmHashingAlgorithm(algorithm);

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
  programFlowBasedOutputSelection();
}

void BcmRtag7Module::programFlowBasedOutputSelection() {
  enableFlowBasedOutputSelection();

  programMacroFlowIDSelection();

  programFlowBasedHashTable();
}

void BcmRtag7Module::enableFlowBasedOutputSelection() {
  int rv = setUnitControl(outputControl_.flowBasedOutputSelection, kEnable);
  bcmCheckError(rv, "failed to enable flow-based output selection");
}

void BcmRtag7Module::programMacroFlowIDSelection() {
  // Note that this is a global configuration ie. it is shared between both
  // module A and module B

  // The following settings are not user-controllable
  auto hashFunction = BcmRtag7Module::getMacroFlowIDHashingAlgorithm();
  int rv =
      setUnitControl(outputControl_.macroFlowIDFunctionControl, hashFunction);
  bcmCheckError(rv, "failed to set macro-flow ID function");

  const int moreSignificantByte = 1;
  rv = setUnitControl(
      outputControl_.macroFlowIDIndexControl, moreSignificantByte);
  bcmCheckError(rv, "failed to set macro-flow ID byte selection");
}

void BcmRtag7Module::programFlowBasedHashTable() {
  // Because both outputs are programmed with the same hashing algorithm (see
  // BcmRtag7Modlue::programAlgorithm), it does not matter which output we
  // use. For simplicity, the first output is always selected.
  int rv = setUnitControl(
      outputControl_.flowBasedHashTableStartingBitIndex,
      moduleControl_.selectFirstOutput.first);
  bcmCheckError(rv, "failed to program flow hash table starting index");

  rv = setUnitControl(
      outputControl_.flowBasedHashTableEndingBitIndex,
      moduleControl_.selectFirstOutput.second);
  bcmCheckError(rv, "failed to program flow hash table ending index");

  const int stride = 1;
  rv = setUnitControl(
      outputControl_.flowBasedHashTableBarrelShiftStride, stride);
  bcmCheckError(rv, "failed to program flow hash table barrel shift stride");

  if (outputControl_.nonUnicastFlowBasedHashTableStartingBitIndex) {
    rv = setUnitControl(
        outputControl_.nonUnicastFlowBasedHashTableStartingBitIndex.value(),
        moduleControl_.selectFirstOutput.first);
    bcmCheckError(
        rv, "failed to program non-unicast flow hash table starting index");
  }

  if (outputControl_.nonUnicastFlowBasedHashTableEndingBitIndex) {
    rv = setUnitControl(
        outputControl_.nonUnicastFlowBasedHashTableEndingBitIndex.value(),
        moduleControl_.selectFirstOutput.second);
    bcmCheckError(
        rv, "failed to program non-unicast flow hash table ending index");
  }

  if (outputControl_.nonUnicastFlowBasedHashTableBarrelShiftStride) {
    rv = setUnitControl(
        outputControl_.nonUnicastFlowBasedHashTableBarrelShiftStride.value(),
        stride);
    bcmCheckError(
        rv,
        "failed to program non-unicast flow hash table barrel shift stride");
  }
}

void BcmRtag7Module::programFieldSelection(const LoadBalancer& loadBalancer) {
  programIPv4FieldSelection(
      loadBalancer.getIPv4Fields(),
      loadBalancer.getTransportFields(),
      loadBalancer.getUdfGroupIds());
  programIPv6FieldSelection(
      loadBalancer.getIPv6Fields(),
      loadBalancer.getTransportFields(),
      loadBalancer.getUdfGroupIds());
  programTerminatedMPLSFieldSelection(loadBalancer);
  programNonTerminatedMPLSFieldSelection(loadBalancer);
  programUdfSelection(loadBalancer.getUdfGroupIds());
}

void BcmRtag7Module::programIPv4FieldSelection(
    LoadBalancer::IPv4FieldsRange v4FieldsRange,
    LoadBalancer::TransportFieldsRange transportFieldsRange,
    const LoadBalancer::UdfGroupIds& udfGroupIds) {
  int arg = computeIPv4Subfields(v4FieldsRange);
  arg |= computeTransportSubfields(transportFieldsRange);
  arg |= computeUdf(udfGroupIds);

  // TODO(samank): why do we set BCM_HASH_FIELD_{SRC,DST}L4 here...
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

void BcmRtag7Module::programUdfSelection(
    const LoadBalancer::UdfGroupIds& udfGroupIds) {
  int enableUdfHash = 0;
  if (udfGroupIds.size()) {
    switch (moduleControl_.module) {
      case 'A':
        enableUdfHash = BCM_HASH_FIELD0_ENABLE_UDFHASH;
        break;
      case 'B':
        enableUdfHash = BCM_HASH_FIELD1_ENABLE_UDFHASH;
        break;
      default:
        folly::assume_unreachable();
    }
  }
  int rv = setUnitControl(bcmSwitchUdfHashEnable, enableUdfHash);
  bcmCheckError(rv, "failed to enable Udf hash");
}

void BcmRtag7Module::programIPv6FieldSelection(
    LoadBalancer::IPv6FieldsRange v6FieldsRange,
    LoadBalancer::TransportFieldsRange transportFieldsRange,
    const LoadBalancer::UdfGroupIds& udfGroupIds) {
  auto it = std::find(
      v6FieldsRange.begin(),
      v6FieldsRange.end(),
      LoadBalancer::IPv6Field::FLOW_LABEL);
  if (it != v6FieldsRange.end()) {
    enableFlowLabelSelection();
  }

  int arg = computeIPv6Subfields(v6FieldsRange);
  arg |= computeTransportSubfields(transportFieldsRange);
  arg |= computeUdf(udfGroupIds);

  // TODO(samank): why do we set BCM_HASH_FIELD_{SRC,DST}L4 here...
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
      rv = setUnitControl(bcmSwitchHashControl, BCM_HASH_CONTROL_ECMP_ENHANCE);
      bcmCheckError(rv, "failed to enable RTAG7 for ECMP");
      break;
    case LoadBalancerID::AGGREGATE_PORT:
      // RTAG7 for trunks is enabled via bcm_trunk_info_t.psc for unicast
      // traffic
      break;
  }
}

void BcmRtag7Module::init(const std::shared_ptr<LoadBalancer>& loadBalancer) {
  // By unconditonally enabling preprocessing, we are keeping with old behavior
  programPreprocessing(kEnable);

  programSeed(loadBalancer->getSeed());

  programFieldSelection(*loadBalancer);

  programAlgorithm(loadBalancer->getAlgorithm());

  programOutputSelection();

  if (!fieldControlProgrammed_) {
    programFieldControl();
    fieldControlProgrammed_ = true;
  }

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
  bool sameMPLSFields = std::equal(
      oldLoadBalancer->getMPLSFields().begin(),
      oldLoadBalancer->getMPLSFields().end(),
      newLoadBalancer->getMPLSFields().begin(),
      newLoadBalancer->getMPLSFields().end());
  bool sameUdf = std::equal(
      oldLoadBalancer->getUdfGroupIds().begin(),
      oldLoadBalancer->getUdfGroupIds().end(),
      newLoadBalancer->getUdfGroupIds().begin(),
      newLoadBalancer->getUdfGroupIds().end());

  if (!sameIPv4Fields || !sameTransportFields || !sameUdf) {
    programIPv4FieldSelection(
        newLoadBalancer->getIPv4Fields(),
        newLoadBalancer->getTransportFields(),
        newLoadBalancer->getUdfGroupIds());
  }

  if (!sameIPv6Fields || !sameTransportFields || !sameUdf) {
    programIPv6FieldSelection(
        newLoadBalancer->getIPv6Fields(),
        newLoadBalancer->getTransportFields(),
        newLoadBalancer->getUdfGroupIds());
  }

  if (!sameUdf) {
    programUdfSelection(newLoadBalancer->getUdfGroupIds());
  }

  if (!sameMPLSFields || !sameIPv4Fields || !sameIPv6Fields) {
    // program for tunnel switch
    programNonTerminatedMPLSFieldSelection(*newLoadBalancer);
  }

  if (!sameIPv4Fields || !sameIPv6Fields || !sameTransportFields) {
    // program for tunnel termination
    programTerminatedMPLSFieldSelection(*newLoadBalancer);
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
    switch (v4Field->cref()) {
      case LoadBalancer::IPv4Field::SOURCE_ADDRESS:
        subfields |= (BCM_HASH_FIELD_IP4SRC_LO | BCM_HASH_FIELD_IP4SRC_HI);
        break;
      case LoadBalancer::IPv4Field::DESTINATION_ADDRESS:
        subfields |= (BCM_HASH_FIELD_IP4DST_LO | BCM_HASH_FIELD_IP4DST_HI);
        break;
    }
  }

  return subfields;
}

int BcmRtag7Module::computeIPv6Subfields(
    LoadBalancer::IPv6FieldsRange v6FieldsRange) const {
  int subfields = 0;

  for (const auto& v6Field : v6FieldsRange) {
    switch (v6Field->cref()) {
      case LoadBalancer::IPv6Field::SOURCE_ADDRESS:
        subfields |= (BCM_HASH_FIELD_IP6SRC_LO | BCM_HASH_FIELD_IP6SRC_HI);
        break;
      case LoadBalancer::IPv6Field::DESTINATION_ADDRESS:
        subfields |= (BCM_HASH_FIELD_IP6DST_LO | BCM_HASH_FIELD_IP6DST_HI);
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
    switch (transportField->cref()) {
      case LoadBalancer::TransportField::SOURCE_PORT:
        subfields |= BCM_HASH_FIELD_SRCL4;
        break;
      case LoadBalancer::TransportField::DESTINATION_PORT:
        subfields |= BCM_HASH_FIELD_DSTL4;
        break;
    }
  }

  return subfields;
}

int BcmRtag7Module::computeUdf(
    const LoadBalancer::UdfGroupIds& udfGroupIds) const {
  int subFields = 0;
  if (udfGroupIds.size()) {
    // udf is configured, since UDF uses bin 2, 3 in the hash
    // they map to srcPort, srcMode fields
    subFields = BCM_HASH_FIELD_SRCMOD | BCM_HASH_FIELD_SRCPORT;
  }
  return subFields;
}

void BcmRtag7Module::programFieldControl() {
  int kDefault = 0;

  // This SDK call controls two types of behavior related to field selection:
  // 1) If shallow-parsing should be enabled for an Ethertype which could
  //    otherwise be deep-parsed.
  // 2) If fields from the inner or outer header should be used in the case
  //    of tunneling.
  auto rv = setUnitControl(bcmSwitchHashSelectControl, kDefault);
  bcmCheckError(rv, "failed to program RTAG7 field control");
}

int BcmRtag7Module::getUnitControl(int unit, bcm_switch_control_t type) {
  int val = 0;

  int rv = bcm_switch_control_get(unit, type, &val);
  bcmCheckError(rv, "failed to retrieve value for ", type);

  return val;
}

template <typename ModuleControlType>
int BcmRtag7Module::setUnitControl(ModuleControlType controlType, int arg) {
  auto wbCache = hw_->getWarmBootCache();

  if (wbCache->unitControlMatches(moduleControl_.module, controlType, arg)) {
    XLOG(DBG2) << "Skipping assigning " << arg << " to " << controlType;

    wbCache->programmed(moduleControl_.module, controlType);

    return BCM_E_NONE;
  }

  return bcm_switch_control_set(hw_->getUnit(), controlType, arg);
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
  state[control.terminatedMPLSFieldSelectionControl] =
      BcmRtag7Module::getUnitControl(
          unit, control.terminatedMPLSFieldSelectionControl);
  state[control.nonTerminatedMPLSFieldSelectionControl] =
      BcmRtag7Module::getUnitControl(
          unit, control.nonTerminatedMPLSFieldSelectionControl);

  state[control.hashFunction1] =
      BcmRtag7Module::getUnitControl(unit, control.hashFunction1);
  state[control.hashFunction2] =
      BcmRtag7Module::getUnitControl(unit, control.hashFunction2);

  return state;
}

void BcmRtag7Module::programTerminatedMPLSFieldSelection(
    const LoadBalancer& loadBalancer) {
  int fields = 0;
  fields |=
      computeL3MPLSPayloadSubfields(loadBalancer, true /* tunnel terminated*/);

  auto rv = setUnitControl(
      moduleControl_.terminatedMPLSFieldSelectionControl, fields);
  bcmCheckError(rv, "failed to config field selection");
}

void BcmRtag7Module::programNonTerminatedMPLSFieldSelection(
    const LoadBalancer& loadBalancer) {
  int fields = 0;
  fields |= computeL3MPLSPayloadSubfields(
      loadBalancer, false /* tunnel not terminated*/);
  fields |= computeL3MPLSHeaderSubfields(loadBalancer);

  auto rv = setUnitControl(
      moduleControl_.nonTerminatedMPLSFieldSelectionControl, fields);
  bcmCheckError(rv, "failed to config field selection");
}

const BcmRtag7Module::OutputSelectionControl
BcmRtag7Module::kEcmpOutputSelectionControl() {
  static const OutputSelectionControl ecmpOutputSelectionControl = {
      LoadBalancerID::ECMP,
      bcmSwitchHashUseFlowSelEcmp,
      bcmSwitchMacroFlowHashFieldConfig,
      bcmSwitchMacroFlowHashUseMSB,
      bcmSwitchMacroFlowHashMinOffset,
      bcmSwitchMacroFlowHashMaxOffset,
      bcmSwitchMacroFlowHashStrideOffset,
      std::nullopt,
      std::nullopt,
      std::nullopt};

  return ecmpOutputSelectionControl;
}

const BcmRtag7Module::OutputSelectionControl
BcmRtag7Module::kTrunkOutputSelectionControl(const HwAsic* asic) {
  if (!asic->isSupported(HwAsic::Feature::NON_UNICAST_HASH)) {
    static const OutputSelectionControl trunkOutputSelectionControl = {
        LoadBalancerID::AGGREGATE_PORT,
        bcmSwitchHashUseFlowSelTrunkUc,
        bcmSwitchMacroFlowHashFieldConfig,
        bcmSwitchMacroFlowHashUseMSB,
        bcmSwitchMacroFlowTrunkHashMinOffset,
        bcmSwitchMacroFlowTrunkHashMaxOffset,
        bcmSwitchMacroFlowTrunkHashStrideOffset,
        std::nullopt,
        std::nullopt,
        std::nullopt};
    return trunkOutputSelectionControl;
  } else {
    static const OutputSelectionControl trunkOutputSelectionControl = {
        LoadBalancerID::AGGREGATE_PORT,
        bcmSwitchHashUseFlowSelTrunkUc,
        bcmSwitchMacroFlowHashFieldConfig,
        bcmSwitchMacroFlowHashUseMSB,
        bcmSwitchMacroFlowTrunkUnicastHashMinOffset,
        bcmSwitchMacroFlowTrunkUnicastHashMaxOffset,
        bcmSwitchMacroFlowTrunkUnicastHashStrideOffset,
        bcmSwitchMacroFlowTrunkNonUnicastHashMinOffset,
        bcmSwitchMacroFlowTrunkNonUnicastHashMaxOffset,
        bcmSwitchMacroFlowTrunkNonUnicastHashStrideOffset};
    return trunkOutputSelectionControl;
  }
}

int BcmRtag7Module::setUnitControl(int controlType, int arg) {
  auto wbCache = hw_->getWarmBootCache();

  if (wbCache->unitControlMatches(outputControl_.id, controlType, arg)) {
    XLOG(DBG2) << "Skipping assigning " << arg << " to " << controlType;

    wbCache->programmed(outputControl_.id, controlType);

    return BCM_E_NONE;
  }

  return bcm_switch_control_set(
      hw_->getUnit(), static_cast<bcm_switch_control_t>(controlType), arg);
}

int BcmRtag7Module::getFlowLabelSubfields() const {
  return BCM_HASH_FIELD_FLOWLABEL_LO | BCM_HASH_FIELD_FLOWLABEL_HI;
}

void BcmRtag7Module::enableFlowLabelSelection() {
  bcm_switch_control_t ipv6EnableFlowLabelSelection;

  switch (moduleControl_.module) {
    case 'A':
      ipv6EnableFlowLabelSelection = bcmSwitchHashField0Ip6FlowLabel;
      break;
    case 'B':
      ipv6EnableFlowLabelSelection = bcmSwitchHashField1Ip6FlowLabel;
      break;
    default:
      folly::assume_unreachable();
  }

  int rv = setUnitControl(
      static_cast<bcm_switch_control_t>(ipv6EnableFlowLabelSelection), kEnable);
  bcmCheckError(rv, "failed to allocate bins 0 and 1 to IPv6 flow label");
}

int BcmRtag7Module::getBcmHashingAlgorithm(
    cfg::HashingAlgorithm algorithm) const {
  switch (algorithm) {
    case cfg::HashingAlgorithm::CRC16_CCITT:
      return BCM_HASH_FIELD_CONFIG_CRC16CCITT;
    case cfg::HashingAlgorithm::CRC32_LO:
      return BCM_HASH_FIELD_CONFIG_CRC32LO;
    case cfg::HashingAlgorithm::CRC32_HI:
      return BCM_HASH_FIELD_CONFIG_CRC32HI;
    case cfg::HashingAlgorithm::CRC32_ETHERNET_LO:
      return BCM_HASH_FIELD_CONFIG_CRC32_ETH_LO;
    case cfg::HashingAlgorithm::CRC32_ETHERNET_HI:
      return BCM_HASH_FIELD_CONFIG_CRC32_ETH_HI;
    case cfg::HashingAlgorithm::CRC32_KOOPMAN_LO:
      return BCM_HASH_FIELD_CONFIG_CRC32_KOOPMAN_LO;
    case cfg::HashingAlgorithm::CRC32_KOOPMAN_HI:
      return BCM_HASH_FIELD_CONFIG_CRC32_KOOPMAN_HI;
  }

  throw FbossError("Unrecognized HashingAlgorithm");
}

int BcmRtag7Module::getMacroFlowIDHashingAlgorithm() {
  return BCM_HASH_FIELD_CONFIG_CRC16XOR8;
}

int BcmRtag7Module::getUnitControl(int unit, int type) {
  int val;

  int rv = bcm_switch_control_get(
      unit, static_cast<bcm_switch_control_t>(type), &val);
  bcmCheckError(rv, "failed to retrieve value for ", type);

  return val;
}

BcmRtag7Module::OutputSelectionState BcmRtag7Module::retrieveRtag7OutputState(
    int unit,
    OutputSelectionControl control) {
  OutputSelectionState state;

  state[control.flowBasedOutputSelection] =
      getUnitControl(unit, control.flowBasedOutputSelection);

  state[control.macroFlowIDFunctionControl] =
      getUnitControl(unit, control.macroFlowIDFunctionControl);

  state[control.macroFlowIDIndexControl] =
      getUnitControl(unit, control.macroFlowIDIndexControl);

  state[control.flowBasedHashTableStartingBitIndex] =
      getUnitControl(unit, control.flowBasedHashTableStartingBitIndex);

  state[control.flowBasedHashTableEndingBitIndex] =
      getUnitControl(unit, control.flowBasedHashTableEndingBitIndex);

  state[control.flowBasedHashTableBarrelShiftStride] =
      getUnitControl(unit, control.flowBasedHashTableBarrelShiftStride);

  if (control.nonUnicastFlowBasedHashTableStartingBitIndex) {
    state[control.nonUnicastFlowBasedHashTableStartingBitIndex.value()] =
        getUnitControl(
            unit, control.nonUnicastFlowBasedHashTableStartingBitIndex.value());
  }
  if (control.nonUnicastFlowBasedHashTableEndingBitIndex) {
    state[control.nonUnicastFlowBasedHashTableEndingBitIndex.value()] =
        getUnitControl(
            unit, control.nonUnicastFlowBasedHashTableEndingBitIndex.value());
  }
  if (control.nonUnicastFlowBasedHashTableBarrelShiftStride) {
    state[control.nonUnicastFlowBasedHashTableBarrelShiftStride.value()] =
        getUnitControl(
            unit,
            control.nonUnicastFlowBasedHashTableBarrelShiftStride.value());
  }
  return state;
}

TerminatedMPLSFieldSelectionControl
BcmRtag7Module::getTerminatedMPLSFieldSelectionControl(char module) {
  CHECK(module == 'A' || module == 'B') << "invalid module " << module;
  return module == 'A'
      ? TerminatedMPLSFieldSelectionControl(
            static_cast<bcm_switch_control_t>(bcmSwitchHashL3MPLSField0))
      : TerminatedMPLSFieldSelectionControl(
            static_cast<bcm_switch_control_t>(bcmSwitchHashL3MPLSField1));
}

NonTerminatedMPLSFieldSelectionControl
BcmRtag7Module::getNonTerminatedMPLSFieldSelectionControl(char module) {
  CHECK(module == 'A' || module == 'B') << "invalid module " << module;
  return module == 'A'
      ? NonTerminatedMPLSFieldSelectionControl(
            static_cast<bcm_switch_control_t>(bcmSwitchHashMPLSTunnelField0))
      : NonTerminatedMPLSFieldSelectionControl(
            static_cast<bcm_switch_control_t>(bcmSwitchHashMPLSTunnelField1));
}

int BcmRtag7Module::computeL3MPLSPayloadSubfields(
    const LoadBalancer& loadBalancer,
    bool forTunnelTermination) {
  int fields = 0;

  for (const auto& v4Field : loadBalancer.getIPv4Fields()) {
    switch (v4Field->cref()) {
      case LoadBalancer::IPv4Field::SOURCE_ADDRESS:
        // upper and lower 16 bits of v4 src address
        fields |=
            (BCM_HASH_MPLS_FIELD_IP4SRC_LO | BCM_HASH_MPLS_FIELD_IP4SRC_HI);
        break;
      case LoadBalancer::IPv4Field::DESTINATION_ADDRESS:
        // upper and lower 16 bits of v4 dst address
        fields |=
            (BCM_HASH_MPLS_FIELD_IP4DST_LO | BCM_HASH_MPLS_FIELD_IP4DST_HI);
        break;
    }
  }

  for (const auto& v6Field : loadBalancer.getIPv6Fields()) {
    switch (v6Field->cref()) {
      case LoadBalancer::IPv6Field::SOURCE_ADDRESS:
        // upper and lower 16 bits of collapsed v6 src address
        fields |=
            (BCM_HASH_MPLS_FIELD_IP4SRC_LO | BCM_HASH_MPLS_FIELD_IP4SRC_HI);
        break;
      case LoadBalancer::IPv6Field::DESTINATION_ADDRESS:
        // upper and lower 16 bits of collapsed v6 dst address
        fields |=
            (BCM_HASH_MPLS_FIELD_IP4DST_LO | BCM_HASH_MPLS_FIELD_IP4DST_HI);
        break;
      case LoadBalancer::IPv6Field::FLOW_LABEL:
        fields |= (BCM_HASH_FIELD_FLOWLABEL_LO | BCM_HASH_FIELD_FLOWLABEL_HI);
        break;
    }
  }

  if (forTunnelTermination) {
    // transport fields are available for hashing terminated L3 MPLS
    fields |= computeTransportSubfields(loadBalancer.getTransportFields());
  } else {
    // flow label is not available for hashing non-terminated L3 MPLS
    fields &= ~(BCM_HASH_FIELD_FLOWLABEL_LO | BCM_HASH_FIELD_FLOWLABEL_HI);
  }
  return fields;
}

int BcmRtag7Module::computeL3MPLSHeaderSubfields(
    const LoadBalancer& loadBalancer) {
  int fields = 0;
  for (auto mplsLabel : loadBalancer.getMPLSFields()) {
    switch (mplsLabel->cref()) {
      case LoadBalancer::MPLSField::TOP_LABEL:
        // pick lower 16 bits and 4 upper bits of top label
        fields |=
            (BCM_HASH_MPLS_FIELD_TOP_LABEL | BCM_HASH_MPLS_FIELD_LABELS_4MSB);
        break;
      case LoadBalancer::MPLSField::SECOND_LABEL:
        // pick lower 16 bits and 4 upper bits second label from top
        fields |=
            (BCM_HASH_MPLS_FIELD_2ND_LABEL | BCM_HASH_MPLS_FIELD_LABELS_4MSB);
        break;
      case LoadBalancer::MPLSField::THIRD_LABEL:
        // pick lower 16 bits third label from top and 4 upper bits
        fields |=
            (BCM_HASH_MPLS_FIELD_3RD_LABEL | BCM_HASH_MPLS_FIELD_LABELS_4MSB);
        break;
    }
  }
  return fields;
}

} // namespace facebook::fboss
