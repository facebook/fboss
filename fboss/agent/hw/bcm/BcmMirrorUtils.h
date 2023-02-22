// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <string>

#include "fboss/agent/state/Mirror.h"

extern "C" {
#include <bcm/field.h>
#include <bcm/mirror.h>
}

namespace facebook::fboss {

bcm_field_action_t directionToBcmAclMirrorAction(MirrorDirection direction);
int directionToBcmPortMirrorFlag(MirrorDirection direction);
int sampleDestinationToBcmPortMirrorSflowFlag(
    cfg::SampleDestination sampleDest);
std::string mirrorDirectionName(MirrorDirection direction);
MirrorDirection bcmAclMirrorActionToDirection(bcm_field_action_t action);

} // namespace facebook::fboss
