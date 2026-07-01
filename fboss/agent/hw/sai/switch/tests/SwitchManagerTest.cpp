/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/AdapterKeySerializers.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/state/LoadBalancer.h"

#include <gtest/gtest.h>

#include <vector>

using namespace facebook::fboss;

class SwitchManagerTest : public ManagerTestBase {
 protected:
  std::shared_ptr<LoadBalancer> makeLb(
      cfg::LoadBalancerID id,
      LoadBalancer::IPv4Fields v4,
      LoadBalancer::IPv6Fields v6 = {},
      LoadBalancer::TransportFields transport = {},
      LoadBalancer::UdfGroupIds udfs = {}) {
    return std::make_shared<LoadBalancer>(
        id,
        cfg::HashingAlgorithm::CRC16_CCITT,
        0 /* seed */,
        v4,
        v6,
        transport,
        LoadBalancer::MPLSFields{},
        udfs);
  }

  sai_object_id_t getEcmpHashV4() {
    return saiApiTable->switchApi().getAttribute(
        saiManagerTable->switchManager().getSwitchSaiId(),
        SaiSwitchTraits::Attributes::EcmpHashV4{});
  }
  sai_object_id_t getEcmpHashV6() {
    return saiApiTable->switchApi().getAttribute(
        saiManagerTable->switchManager().getSwitchSaiId(),
        SaiSwitchTraits::Attributes::EcmpHashV6{});
  }
  sai_object_id_t getLagHashV4() {
    return saiApiTable->switchApi().getAttribute(
        saiManagerTable->switchManager().getSwitchSaiId(),
        SaiSwitchTraits::Attributes::LagHashV4{});
  }
  sai_object_id_t getLagHashV6() {
    return saiApiTable->switchApi().getAttribute(
        saiManagerTable->switchManager().getSwitchSaiId(),
        SaiSwitchTraits::Attributes::LagHashV6{});
  }
};

// Regression test: setting hash-fields to empty ("none") must clear the SAI
// switch attribute rather than silently leaving the old hash programmed.
TEST_F(SwitchManagerTest, clearEcmpHashFieldsClearsSwitch) {
  auto lbWithFields = makeLb(
      cfg::LoadBalancerID::ECMP,
      {cfg::IPv4Field::SOURCE_ADDRESS, cfg::IPv4Field::DESTINATION_ADDRESS},
      {cfg::IPv6Field::SOURCE_ADDRESS, cfg::IPv6Field::DESTINATION_ADDRESS});
  saiManagerTable->switchManager().addOrUpdateLoadBalancer(lbWithFields);

  EXPECT_NE(getEcmpHashV4(), SAI_NULL_OBJECT_ID);
  EXPECT_NE(getEcmpHashV6(), SAI_NULL_OBJECT_ID);

  // Clear both field sets to empty.
  auto lbCleared = makeLb(cfg::LoadBalancerID::ECMP, {}, {});
  saiManagerTable->switchManager().changeLoadBalancer(lbWithFields, lbCleared);

  EXPECT_EQ(getEcmpHashV4(), SAI_NULL_OBJECT_ID);
  EXPECT_EQ(getEcmpHashV6(), SAI_NULL_OBJECT_ID);
}

TEST_F(SwitchManagerTest, clearLagHashFieldsClearsSwitch) {
  auto lbWithFields = makeLb(
      cfg::LoadBalancerID::AGGREGATE_PORT,
      {cfg::IPv4Field::SOURCE_ADDRESS, cfg::IPv4Field::DESTINATION_ADDRESS},
      {cfg::IPv6Field::SOURCE_ADDRESS, cfg::IPv6Field::DESTINATION_ADDRESS});
  saiManagerTable->switchManager().addOrUpdateLoadBalancer(lbWithFields);

  EXPECT_NE(getLagHashV4(), SAI_NULL_OBJECT_ID);
  EXPECT_NE(getLagHashV6(), SAI_NULL_OBJECT_ID);

  auto lbCleared = makeLb(cfg::LoadBalancerID::AGGREGATE_PORT, {}, {});
  saiManagerTable->switchManager().changeLoadBalancer(lbWithFields, lbCleared);

  EXPECT_EQ(getLagHashV4(), SAI_NULL_OBJECT_ID);
  EXPECT_EQ(getLagHashV6(), SAI_NULL_OBJECT_ID);
}

// Regression test: clearing only IPv4 fields must not disturb IPv6.
TEST_F(SwitchManagerTest, clearEcmpHashV4OnlyLeavesV6Intact) {
  auto lbFull = makeLb(
      cfg::LoadBalancerID::ECMP,
      {cfg::IPv4Field::SOURCE_ADDRESS},
      {cfg::IPv6Field::SOURCE_ADDRESS});
  saiManagerTable->switchManager().addOrUpdateLoadBalancer(lbFull);

  auto v6Before = getEcmpHashV6();
  EXPECT_NE(v6Before, SAI_NULL_OBJECT_ID);

  // Clear only IPv4 fields; keep IPv6 unchanged.
  auto lbV4Cleared =
      makeLb(cfg::LoadBalancerID::ECMP, {}, {cfg::IPv6Field::SOURCE_ADDRESS});
  saiManagerTable->switchManager().changeLoadBalancer(lbFull, lbV4Cleared);

  EXPECT_EQ(getEcmpHashV4(), SAI_NULL_OBJECT_ID);
  EXPECT_EQ(getEcmpHashV6(), v6Before);
}

TEST_F(SwitchManagerTest, serDeser) {
  auto saiSwitch = static_cast<SaiSwitch*>(saiPlatform->getHwSwitch());
  auto adapterKeys = saiSwitch->toFollyDynamic();
  std::vector<typename SaiSwitchTraits::AdapterKey> switchKeys;
  for (auto obj : adapterKeys[kAdapterKeys][saiObjectTypeToString(
           SaiSwitchTraits::ObjectType)]) {
    switchKeys.push_back(fromFollyDynamic<SaiSwitchTraits>(obj));
  }
  EXPECT_EQ(
      std::vector<SaiSwitchTraits::AdapterKey>{saiSwitch->getSaiSwitchId()},
      switchKeys);
}
