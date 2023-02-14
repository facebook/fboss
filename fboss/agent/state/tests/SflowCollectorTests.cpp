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
  validateThriftMapMapSerialization(sflowCollectorMap);
}
