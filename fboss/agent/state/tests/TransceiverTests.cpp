/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Transceiver.h"
#include "fboss/agent/state/TransceiverMap.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

TEST(Transceiver, SerializeTransceiver) {
  auto tcvr = std::make_unique<Transceiver>(TransceiverID(1));
  tcvr->setCableLength(3.5);
  tcvr->setMediaInterface(MediaInterfaceCode::FR1_100G);
  tcvr->setManagementInterface(TransceiverManagementInterface::SFF);

  auto serialized = tcvr->toFollyDynamic();
  auto tcvrBack = Transceiver::fromFollyDynamic(serialized);

  EXPECT_TRUE(*tcvr == *tcvrBack);
}

TEST(Transceiver, SerializeSwitchState) {
  auto platform = createMockPlatform();
  auto state = std::make_shared<SwitchState>();

  auto tcvr1 = std::make_shared<Transceiver>(TransceiverID(1));
  tcvr1->setCableLength(3.5);
  tcvr1->setMediaInterface(MediaInterfaceCode::FR1_100G);
  tcvr1->setManagementInterface(TransceiverManagementInterface::SFF);

  auto tcvr2 = std::make_shared<Transceiver>(TransceiverID(2));
  tcvr2->setCableLength(1.5);
  tcvr2->setMediaInterface(MediaInterfaceCode::CWDM4_100G);
  tcvr2->setManagementInterface(TransceiverManagementInterface::CMIS);

  state->addTransceiver(tcvr1);
  state->addTransceiver(tcvr2);

  auto serialized = state->toFollyDynamic();
  auto stateBack = SwitchState::fromFollyDynamic(serialized);

  // Check all transceivers should be there
  for (auto tcvrID : {TransceiverID(1), TransceiverID(2)}) {
    EXPECT_TRUE(
        *state->getTransceivers()->getTransceiver(tcvrID) ==
        *stateBack->getTransceivers()->getTransceiver(tcvrID));
  }
}
