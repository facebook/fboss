/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/state/PortDescriptor.h"

#include <gtest/gtest.h>

TEST(PortDescriptor, TestPhysicalEqualPhysical) {
  facebook::fboss::PortID p1(42);
  facebook::fboss::PortID p2(42);
  facebook::fboss::PortDescriptor pd1(p1);
  facebook::fboss::PortDescriptor pd2(p2);
  EXPECT_EQ(pd1, pd2);
}

TEST(PortDescriptor, TestPhysicalNotEqualPhysical) {
  facebook::fboss::PortID p1(42);
  facebook::fboss::PortID p2(43);
  facebook::fboss::PortDescriptor pd1(p1);
  facebook::fboss::PortDescriptor pd2(p2);
  EXPECT_NE(pd1, pd2);
  EXPECT_LT(pd1, pd2);
}

TEST(PortDescriptor, TestAggregateEqualAggregate) {
  facebook::fboss::AggregatePortID ap1(42);
  facebook::fboss::AggregatePortID ap2(42);
  facebook::fboss::PortDescriptor pd1(ap1);
  facebook::fboss::PortDescriptor pd2(ap2);
  EXPECT_EQ(pd1, pd2);
}

TEST(PortDescriptor, TestAggregateNotEqualAggregate) {
  facebook::fboss::AggregatePortID ap1(42);
  facebook::fboss::AggregatePortID ap2(43);
  facebook::fboss::PortDescriptor pd1(ap1);
  facebook::fboss::PortDescriptor pd2(ap2);
  EXPECT_NE(pd1, pd2);
  EXPECT_LT(pd1, pd2);
}

TEST(PortDescriptor, TestPhysicalNotEqualAggregate) {
  facebook::fboss::PortID p(42);
  facebook::fboss::AggregatePortID ap(42);
  facebook::fboss::PortDescriptor pd1(p);
  facebook::fboss::PortDescriptor pd2(ap);
  EXPECT_NE(pd1, pd2);
  EXPECT_LT(pd1, pd2);
}

TEST(PortDescriptor, TestGetters) {
  facebook::fboss::PortID pp(42);
  facebook::fboss::AggregatePortID ap(42);
  facebook::fboss::SystemPortID sp(42);
  facebook::fboss::PortDescriptor ppd(pp);
  facebook::fboss::PortDescriptor apd(ap);
  facebook::fboss::PortDescriptor spd(sp);
  // Physical port
  EXPECT_EQ(pp, ppd.phyPortID());
  EXPECT_EQ(pp, *ppd.phyPortIDIf());
  EXPECT_EQ(std::nullopt, ppd.aggPortIDIf());
  EXPECT_EQ(std::nullopt, ppd.sysPortIDIf());
  // Agg port
  EXPECT_EQ(ap, apd.aggPortID());
  EXPECT_EQ(ap, *apd.aggPortIDIf());
  EXPECT_EQ(std::nullopt, apd.phyPortIDIf());
  EXPECT_EQ(std::nullopt, apd.sysPortIDIf());
  // System port
  EXPECT_EQ(sp, spd.sysPortID());
  EXPECT_EQ(sp, *spd.sysPortIDIf());
  EXPECT_EQ(std::nullopt, spd.phyPortIDIf());
  EXPECT_EQ(std::nullopt, spd.aggPortIDIf());
}
