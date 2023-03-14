/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FbossError.h"
#include "fboss/agent/FsdbHelper.h"
#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <boost/container/flat_map.hpp>
#include <gtest/gtest.h>

using namespace facebook::fboss;
using boost::container::flat_map;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

TEST(OperDeltaTests, OperDeltaCompute) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  auto config = testConfigA();

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);

  auto delta1 = StateDelta(stateV0, stateV1);
  // 20 ports, 2 vlans, 2 intfs
  EXPECT_EQ(delta1.getOperDelta().changes()->size(), 24);

  auto stateV2 = stateV1->clone();
  auto mirrors = stateV2->getMirrors()->clone();
  state::MirrorFields mirror{};
  mirror.name() = "mirror0";
  mirror.configHasEgressPort() = true;
  mirror.egressPort() = 1;
  mirror.isResolved() = true;
  mirrors->addMirror(std::make_shared<Mirror>(mirror));
  stateV2->resetMirrors(mirrors);
  stateV2->publish();

  auto delta2 = StateDelta(stateV1, stateV2);
  // 1 mirror
  EXPECT_EQ(delta2.getOperDelta().changes()->size(), 1);
}
