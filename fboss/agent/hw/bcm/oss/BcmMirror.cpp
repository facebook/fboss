// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmMirror.h"

namespace facebook {
namespace fboss {

BcmMirrorDestination::BcmMirrorDestination(
    int /*unit*/,
    BcmPort* /*egressPort*/) {}

BcmMirrorDestination::BcmMirrorDestination(
    int /*unit*/,
    BcmPort* /*egressPort*/,
    const MirrorTunnel& /*mirrorTunnel*/) {}

BcmMirrorDestination::~BcmMirrorDestination() {}

BcmMirrorHandle BcmMirrorDestination::getHandle() {
  return handle_;
}

BcmMirror::BcmMirror(
    BcmSwitch* /*hw*/,
    const std::shared_ptr<Mirror>& /*mirror */) {}

BcmMirror::~BcmMirror() {}

void BcmMirror::program() {}

void applyPortMirrorAction(
    PortID /*port*/,
    MirrorAction /*action*/,
    MirrorDirection /*direction*/) {}
void applyAclMirrorAction(
    BcmAclEntryHandle /* aclEntryHandle*/,
    MirrorAction /*action*/,
    MirrorDirection /*direction*/) {}
} // namespace fboss
} // namespace facebook
