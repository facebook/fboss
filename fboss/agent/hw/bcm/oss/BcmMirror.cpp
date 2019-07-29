// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmMirror.h"

namespace facebook {
namespace fboss {

BcmMirrorDestination::BcmMirrorDestination(
    int /*unit*/,
    BcmPort* /*egressPort*/,
    uint8_t /* dscp */) {}

BcmMirrorDestination::BcmMirrorDestination(
    int /*unit*/,
    BcmPort* /*egressPort*/,
    const MirrorTunnel& /*mirrorTunnel*/,
    uint8_t /*dscp*/) {}

BcmMirrorDestination::~BcmMirrorDestination() {}

BcmMirrorHandle BcmMirrorDestination::getHandle() {
  return handle_;
}

BcmMirror::BcmMirror(
    BcmSwitch* /*hw*/,
    const std::shared_ptr<Mirror>& /*mirror */) {}

BcmMirror::~BcmMirror() {}

void BcmMirror::program(const std::shared_ptr<Mirror>& /*mirror*/) {}

void BcmMirror::applyPortMirrorAction(
    PortID /*port*/,
    MirrorAction /*action*/,
    MirrorDirection /*direction*/,
    cfg::SampleDestination /*sampleDest*/) {}
void BcmMirror::applyAclMirrorAction(
    BcmAclEntryHandle /* aclEntryHandle*/,
    MirrorAction /*action*/,
    MirrorDirection /*direction*/) {}

void BcmMirror::applyPortMirrorActions(MirrorAction /*action*/) {}
void BcmMirror::applyAclMirrorActions(MirrorAction /*action*/) {}
} // namespace fboss
} // namespace facebook
