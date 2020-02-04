/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/StatsConstants.h"

#include <fb303/ServiceData.h>

#include <gtest/gtest.h>
using namespace facebook::fboss;
using namespace facebook::fb303;

TEST(HwPortFb303StatsTest, StatsInit) {
  HwPortFb303Stats stats("eth1/1/1");
  for (const auto& statName : stats.statNames()) {
    EXPECT_TRUE(fbData->getStatMap()->contains(statName));
  }
}

TEST(HwPortFb303StatsTest, StatsDeInit) {
  std::vector<std::string> statNames;
  {
    HwPortFb303Stats stats("eth1/1/1");
    statNames = stats.statNames();
  }
  for (const auto& statName : statNames) {
    EXPECT_FALSE(fbData->getStatMap()->contains(statName));
  }
}
