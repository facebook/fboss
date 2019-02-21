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

#include "fboss/agent/state/Port.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

namespace facebook {
namespace fboss {

void ManagerTestBase::SetUp() {
  fs = FakeSai::getInstance();
  saiApiTable = std::make_unique<SaiApiTable>();
  saiManagerTable = std::make_unique<SaiManagerTable>(saiApiTable.get());
  sai_api_initialize(0, nullptr);
}

sai_object_id_t ManagerTestBase::addPort(
    const PortID& swId,
    const std::string& name,
    bool enabled) {
  auto swPort = std::make_shared<Port>(swId, name);
  swPort->setSpeed(cfg::PortSpeed::TWENTYFIVEG);
  if (enabled) {
    swPort->setAdminState(cfg::PortState::ENABLED);
  }
  sai_object_id_t saiId = saiManagerTable->portManager().addPort(swPort);
  return saiId;
}

} // namespace fboss
} // namespace facebook
