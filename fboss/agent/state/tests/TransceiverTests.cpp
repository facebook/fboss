/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/Transceiver.h"

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
