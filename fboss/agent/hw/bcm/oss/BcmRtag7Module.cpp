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
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmError.h"

namespace facebook::fboss {

const BcmRtag7Module::OutputSelectionControl
BcmRtag7Module::kEcmpOutputSelectionControl() {
  static const OutputSelectionControl ecmpOutputSelectionControl = {
      LoadBalancerID::ECMP,
  };
  return ecmpOutputSelectionControl;
}

const BcmRtag7Module::OutputSelectionControl
BcmRtag7Module::kTrunkOutputSelectionControl() {
  static const OutputSelectionControl trunkOutputSelectionControl = {
      LoadBalancerID::AGGREGATE_PORT,
  };
  return trunkOutputSelectionControl;
}

int BcmRtag7Module::getFlowLabelSubfields() const {
  throw FbossError("Flow label symbols not exported by Bcm");
  return 0;
}

void BcmRtag7Module::enableFlowLabelSelection() {
  throw FbossError("Flow label symbols not exported by Bcm");
}

int BcmRtag7Module::getBcmHashingAlgorithm(
    cfg::HashingAlgorithm algorithm) const {
  if (algorithm == cfg::HashingAlgorithm::CRC16_CCITT) {
    return BCM_HASH_FIELD_CONFIG_CRC16CCITT;
  }

  throw FbossError("Unrecognized HashingAlgorithm");
}

int BcmRtag7Module::getMacroFlowIDHashingAlgorithm() {
  throw FbossError("No XOR-folded hash functions exported by Bcm");
  return 0;
}

int BcmRtag7Module::setUnitControl(int controlType, int arg) {
  return bcm_switch_control_set(
      hw_->getUnit(), static_cast<bcm_switch_control_t>(controlType), arg);
}

int BcmRtag7Module::getUnitControl(int unit, int type) {
  int val;

  int rv = bcm_switch_control_get(
      unit, static_cast<bcm_switch_control_t>(type), &val);
  bcmCheckError(rv, "failed to retrieve value for ", type);

  return val;
}

BcmRtag7Module::OutputSelectionState BcmRtag7Module::retrieveRtag7OutputState(
    int /* unit */,
    OutputSelectionControl /* control */) {
  return OutputSelectionState();
}

TerminatedMPLSFieldSelectionControl
BcmRtag7Module::getTerminatedMPLSFieldSelectionControl(char /*module*/) {
  return TerminatedMPLSFieldSelectionControl();
}

NonTerminatedMPLSFieldSelectionControl
BcmRtag7Module::getNonTerminatedMPLSFieldSelectionControl(char /*module*/) {
  return NonTerminatedMPLSFieldSelectionControl();
}

int BcmRtag7Module::computeL3MPLSPayloadSubfields(
    const LoadBalancer& /*loadBalancer*/,
    bool /*forTunnelTermination*/) {
  return 0;
}
int BcmRtag7Module::computeL3MPLSHeaderSubfields(
    const LoadBalancer& /*loadBalancer*/) {
  return 0;
}

} // namespace facebook::fboss
