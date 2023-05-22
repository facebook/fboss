/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/Port.h"

#include <folly/Conv.h>
#include "fboss/agent/state/BufferPoolConfig.h"
#include "fboss/agent/state/PortPgConfig.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>
#include "fboss/agent/state/NodeBase-defs.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

using apache::thrift::TEnumTraits;
using folly::to;
using std::string;

namespace facebook::fboss {

state::RxSak PortFields::rxSakToThrift(
    const PortFields::MKASakKey& sakKey,
    const mka::MKASak& sak) {
  state::MKASakKey sakKeyThrift;
  *sakKeyThrift.sci() = sakKey.sci;
  *sakKeyThrift.associationNum() = sakKey.associationNum;
  state::RxSak rxSakThrift;
  *rxSakThrift.sakKey() = sakKeyThrift;
  *rxSakThrift.sak() = sak;
  return rxSakThrift;
}

std::pair<PortFields::MKASakKey, mka::MKASak> PortFields::rxSakFromThrift(
    state::RxSak rxSak) {
  PortFields::MKASakKey sakKey{
      *rxSak.sakKey()->sci(), *rxSak.sakKey()->associationNum()};
  return std::make_pair(sakKey, *rxSak.sak());
}

state::VlanInfo PortFields::VlanInfo::toThrift() const {
  state::VlanInfo vlanThrift;
  *vlanThrift.tagged() = tagged;
  return vlanThrift;
}

// static
PortFields::VlanInfo PortFields::VlanInfo::fromThrift(
    const state::VlanInfo& vlanThrift) {
  return VlanInfo(*vlanThrift.tagged());
}

Port* Port::modify(std::shared_ptr<SwitchState>* state) {
  if (!isPublished()) {
    CHECK(!(*state)->isPublished());
    return this;
  }

  MultiSwitchPortMap* ports = (*state)->getPorts()->modify(state);
  const auto scope = ports->getNodeAndScope(getID()).second;
  auto newPort = clone();
  auto* ptr = newPort.get();
  ports->updateNode(std::move(newPort), scope);
  return ptr;
}

Port::Port(PortID id, const std::string& name) {
  set<switch_state_tags::portId>(id);
  set<switch_state_tags::portName>(name);
}

void Port::fillPhyInfo(phy::PhyInfo* phyInfo) {
  phyInfo->name() = getName();
  phyInfo->speed() = getSpeed();
}

InterfaceID Port::getInterfaceID() const {
  // On VOQ/Fabric switches, port and interface have 1:1 relation.
  // For non VOQ/Fabric switches, in practice, a port is always part of a
  // single VLAN (and thus single interface).
  auto intfs = getInterfaceIDs();
  CHECK_EQ(intfs.size(), 1);
  return InterfaceID(intfs.at(0));
}

template class ThriftStructNode<Port, state::PortFields>;

} // namespace facebook::fboss
