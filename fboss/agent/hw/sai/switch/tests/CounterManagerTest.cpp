// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiCounterManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

class CounterManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::PORT | SetupStage::VLAN | SetupStage::INTERFACE;
    ManagerTestBase::SetUp();
    saiManagerTable->counterManager().setMaxRouteCounterIDs(10);
  }
};

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)

TEST_F(CounterManagerTest, addRouteCounter) {
  auto handle =
      saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.0");
  EXPECT_NE(handle, nullptr);
  EXPECT_EQ(handle->counterName, "counter.0");
}

TEST_F(CounterManagerTest, addMultipleRouteCounters) {
  auto handle0 =
      saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.0");
  auto handle1 =
      saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.1");
  EXPECT_NE(handle0, nullptr);
  EXPECT_NE(handle1, nullptr);
  EXPECT_NE(handle0->adapterKey(), handle1->adapterKey());
}

TEST_F(CounterManagerTest, refcountRouteCounter) {
  auto handle0 =
      saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.0");
  auto handle1 =
      saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.0");
  // Same counter ID should return the same handle
  EXPECT_EQ(handle0->adapterKey(), handle1->adapterKey());
}

TEST_F(CounterManagerTest, getHwSwitchCounterStatsEmpty) {
  auto stats = saiManagerTable->counterManager().getHwSwitchCounterStats();
  EXPECT_TRUE(stats.routeCounters()->empty());
}

TEST_F(CounterManagerTest, getHwSwitchCounterStatsWithCounters) {
  auto handle0 =
      saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.0");
  auto handle1 =
      saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.1");

  // Set bytes directly on the handle to simulate stats collection
  handle0->hwSwitchCounter.bytes() = 100;
  handle1->hwSwitchCounter.bytes() = 200;

  auto stats = saiManagerTable->counterManager().getHwSwitchCounterStats();
  EXPECT_EQ(stats.routeCounters()->size(), 2);
  EXPECT_TRUE(stats.routeCounters()->count("counter.0"));
  EXPECT_TRUE(stats.routeCounters()->count("counter.1"));

  EXPECT_EQ(*stats.routeCounters()->at("counter.0").bytes(), 100);
  EXPECT_EQ(*stats.routeCounters()->at("counter.1").bytes(), 200);
}

TEST_F(CounterManagerTest, getHwSwitchCounterStatsAfterRemove) {
  {
    auto handle0 =
        saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.0");
    auto handle1 =
        saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.1");

    auto stats = saiManagerTable->counterManager().getHwSwitchCounterStats();
    EXPECT_EQ(stats.routeCounters()->size(), 2);
    // handle1 goes out of scope — refcount drops to 0, counter removed
  }
  // handle0 also out of scope

  auto stats = saiManagerTable->counterManager().getHwSwitchCounterStats();
  EXPECT_TRUE(stats.routeCounters()->empty());
}

TEST_F(CounterManagerTest, getStatsForCounter) {
  auto handle =
      saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.0");

  // Set bytes directly to simulate stats update
  handle->hwSwitchCounter.bytes() = 42;

  auto stats = saiManagerTable->counterManager().getHwSwitchCounterStats();
  EXPECT_EQ(*stats.routeCounters()->at("counter.0").bytes(), 42);
}

TEST_F(CounterManagerTest, exceedMaxRouteCounterIDs) {
  saiManagerTable->counterManager().setMaxRouteCounterIDs(2);

  auto handle0 =
      saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.0");
  auto handle1 =
      saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.1");
  EXPECT_THROW(
      saiManagerTable->counterManager().incRefOrAddRouteCounter("counter.2"),
      FbossError);
}

TEST_F(CounterManagerTest, counterLabelTooLong) {
  std::string longLabel(33, 'x'); // exceeds max 32
  EXPECT_THROW(
      saiManagerTable->counterManager().incRefOrAddRouteCounter(longLabel),
      FbossError);
}

#endif
