/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/Vlan.h"

namespace facebook {
namespace fboss {

void ManagerTestBase::SetUp() {
  fs = FakeSai::getInstance();
  saiApiTable = std::make_unique<SaiApiTable>();
  saiManagerTable = std::make_unique<SaiManagerTable>(saiApiTable.get());
  sai_api_initialize(0, nullptr);
}

sai_object_id_t ManagerTestBase::addPort(uint16_t id, bool enabled) {
  std::string name = folly::sformat("port{}", id);
  auto swPort = std::make_shared<Port>(PortID(id), name);
  swPort->setSpeed(cfg::PortSpeed::TWENTYFIVEG);
  if (enabled) {
    swPort->setAdminState(cfg::PortState::ENABLED);
  }
  sai_object_id_t saiId = saiManagerTable->portManager().addPort(swPort);
  return saiId;
}

std::shared_ptr<Vlan> ManagerTestBase::makeVlan(
    uint16_t id,
    const std::vector<uint16_t>& memberPorts) const {
  std::string name = folly::sformat("vlan{}", id);
  auto swVlan = std::make_shared<Vlan>(VlanID(id), "vlan42");
  VlanFields::MemberPorts mps;
  for (int memberPort : memberPorts) {
    PortID portId(memberPort);
    VlanFields::PortInfo portInfo(false);
    mps.insert(std::make_pair(portId, portInfo));
  }
  swVlan->setPorts(mps);
  return swVlan;
}

sai_object_id_t ManagerTestBase::addVlan(
    uint16_t id,
    const std::vector<uint16_t>& memberPorts) {
  auto swVlan = makeVlan(id, memberPorts);
  return saiManagerTable->vlanManager().addVlan(swVlan);
}

} // namespace fboss
} // namespace facebook
