// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmMirrorUtils.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

bcm_field_action_t directionToBcmAclMirrorAction(MirrorDirection direction) {
  switch (direction) {
    case MirrorDirection::INGRESS:
      return bcmFieldActionMirrorIngress;
    case MirrorDirection::EGRESS:
      return bcmFieldActionMirrorEgress;
  }
  throw FbossError("Invalid MirrorDirection ", direction);
}

MirrorDirection bcmAclMirrorActionToDirection(bcm_field_action_t action) {
  switch (action) {
    case bcmFieldActionMirrorIngress:
      return MirrorDirection::INGRESS;
    case bcmFieldActionMirrorEgress:
      return MirrorDirection::EGRESS;
    default:
      throw FbossError("Invalid mirror actioon ", action);
  }
}

int directionToBcmPortMirrorFlag(MirrorDirection direction) {
  switch (direction) {
    case MirrorDirection::INGRESS:
      return BCM_MIRROR_PORT_INGRESS;
    case MirrorDirection::EGRESS:
      return BCM_MIRROR_PORT_EGRESS;
  }
  throw FbossError("Invalid MirrorDirection ", direction);
}

/* If sampling destination is set to MIRROR, the switch will forward packets to
 * all mirror destinations with BCM_MIRROR_PORT_SFLOW flag, so we add it here.
 * If we are sampling to CPU then no flag is required.
 */
int sampleDestinationToBcmPortMirrorSflowFlag(
    cfg::SampleDestination sampleDest) {
  if (sampleDest != cfg::SampleDestination::MIRROR) {
    return 0;
  }
  return BCM_MIRROR_PORT_SFLOW;
}

std::string mirrorDirectionName(MirrorDirection direction) {
  switch (direction) {
    case MirrorDirection::INGRESS:
      return "ingress";
    case MirrorDirection::EGRESS:
      return "egress";
  }
  throw FbossError("Invalid MirrorDirection ", direction);
}

} // namespace facebook::fboss
