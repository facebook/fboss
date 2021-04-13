/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/normalization/CounterTagManager.h"

using namespace ::testing;

namespace facebook::fboss::normalization {
namespace {
void addPort(
    std::vector<cfg::Port>& ports,
    const std::string& name,
    const std::vector<std::string>& tags) {
  cfg::Port port;
  port.name_ref() = name;
  port.counterTags_ref() = tags;
  ports.push_back(port);
}
} // namespace

TEST(CounterTagManagerTest, getCounterTags) {
  CounterTagManager counterTagManager;
  cfg::SwitchConfig config;
  std::vector<cfg::Port> ports;

  addPort(ports, "eth1", {"tag_a", "tag_b"});
  addPort(ports, "eth2", {"tag_c", "tag_d"});
  config.ports_ref() = ports;

  // initial load
  counterTagManager.reloadCounterTags(config);
  EXPECT_THAT(
      *counterTagManager.getCounterTags("eth1"), ElementsAre("tag_a", "tag_b"));
  EXPECT_THAT(
      *counterTagManager.getCounterTags("eth2"), ElementsAre("tag_c", "tag_d"));

  // config updated
  ports.clear();
  addPort(ports, "eth1", {"tag_aa", "tag_bb"});
  addPort(ports, "eth2", {"tag_cc", "tag_dd"});
  config.ports_ref() = ports;

  counterTagManager.reloadCounterTags(config);
  EXPECT_THAT(
      *counterTagManager.getCounterTags("eth1"),
      ElementsAre("tag_aa", "tag_bb"));
  EXPECT_THAT(
      *counterTagManager.getCounterTags("eth2"),
      ElementsAre("tag_cc", "tag_dd"));
}

} // namespace facebook::fboss::normalization
