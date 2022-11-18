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
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/SflowCollectorMap.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

TEST(SflowCollector, SerializeSflowCollector) {
  auto sflowCollector = std::make_unique<SflowCollector>(
      std::string("1.2.3.4"), static_cast<uint16_t>(8080));
  auto serialized = sflowCollector->toFollyDynamic();
  auto sflowCollectorBack = SflowCollector::fromFollyDynamic(serialized);

  EXPECT_TRUE(*sflowCollector == *sflowCollectorBack);

  validateNodeSerialization(*sflowCollector);
}

TEST(SflowCollectorMap, SerializeSflowCollectorMap) {
  auto sflowCollector1 = std::make_shared<SflowCollector>(
      std::string("1.2.3.4"), static_cast<uint16_t>(8080));
  auto sflowCollector2 = std::make_shared<SflowCollector>(
      std::string("2::3"), static_cast<uint16_t>(9090));
  SflowCollectorMap sflowCollectorMap;
  sflowCollectorMap.addNode(sflowCollector1);
  sflowCollectorMap.addNode(sflowCollector2);

  EXPECT_EQ(sflowCollectorMap.size(), 2);
  EXPECT_EQ(
      sflowCollectorMap.getNode(sflowCollector1->getID())->getAddress(),
      folly::SocketAddress("1.2.3.4", 8080));
  EXPECT_EQ(
      sflowCollectorMap.getNode(sflowCollector2->getID())->getAddress(),
      folly::SocketAddress("2::3", 9090));

  auto serialized = sflowCollectorMap.toFollyDynamic();
  auto sflowCollectorMapBack = SflowCollectorMap::fromFollyDynamic(serialized);
  EXPECT_TRUE(
      sflowCollectorMap.toThrift() == sflowCollectorMapBack->toThrift());
}
