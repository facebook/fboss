/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmTrunk.h"

namespace facebook {
namespace fboss {

BcmTrunk::~BcmTrunk() {}
void BcmTrunk::init(const std::shared_ptr<AggregatePort>& /*aggPort*/) {}
void BcmTrunk::program(
    const std::shared_ptr<AggregatePort>& /*oldAggPort*/,
    const std::shared_ptr<AggregatePort>& /*newAggPort*/) {}
void BcmTrunk::modifyMemberPortChecked(bool /*added*/, PortID /*memberPort*/) {}
void BcmTrunk::modifyMemberPort(bool /* added */, PortID /* memberPort */) {}

void BcmTrunk::programSubports(
    AggregatePort::SubportsConstRange /* oldMembersRange */,
    AggregatePort::SubportsConstRange /* newMembersRange */) {}
void BcmTrunk::programForwardingState(
    AggregatePort::SubportAndForwardingStateConstRange /* oldRange */,
    AggregatePort::SubportAndForwardingStateConstRange /* newRange */) {}

void BcmTrunk::shrinkTrunkGroupHwNotLocked(
    int /* unit */,
    opennsl_trunk_t /* trunk */,
    opennsl_port_t /* toDisable */) {}
int BcmTrunk::getEnabledMemberPortsCountHwNotLocked(
    int /* unit */,
    opennsl_trunk_t /* trunk */,
    opennsl_port_t /* port */) {
  return 0;
}
folly::Optional<int> BcmTrunk::findTrunk(
    int /* unit */,
    opennsl_module_t /* modid */,
    opennsl_port_t /* port */) {
  return folly::none;
}

opennsl_gport_t BcmTrunk::asGPort(opennsl_trunk_t /* trunk */) {
  return static_cast<opennsl_gport_t>(0);
}
bool BcmTrunk::isValidTrunkPort(opennsl_gport_t /* gPort */) {
  return false;
}

}
} // namespace facebook::fboss
