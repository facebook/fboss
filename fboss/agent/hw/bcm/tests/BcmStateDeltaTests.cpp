/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook {
namespace fboss {

TEST_F(BcmTest, addPortFails) {
  const auto& portMap = getProgrammedState()->getPorts();
  auto highestPortIdPort = *std::max_element(
      portMap->begin(),
      portMap->end(),
      [=](const auto& lport, const auto& rport) {
        return lport->getID() < rport->getID();
      });
  auto newState = getProgrammedState()->clone();
  auto newPortMap = newState->getPorts()->modify(&newState);
  newPortMap->addPort(
      std::make_shared<Port>(PortID(highestPortIdPort->getID() + 1), "foo"));
  EXPECT_THROW(applyNewState(newState), FbossError);
}

TEST_F(BcmTest, removePortFails) {
  const auto& portMap = getProgrammedState()->getPorts();
  auto firstPort = *portMap->begin();
  auto newState = getProgrammedState()->clone();
  auto newPortMap = newState->getPorts()->modify(&newState);
  newPortMap->removeNode(firstPort->getID());
  EXPECT_THROW(applyNewState(newState), FbossError);
}
} // namespace fboss
} // namespace facebook
