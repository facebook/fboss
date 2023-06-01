/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <fboss/agent/gen-cpp2/switch_config_types.h>
#include "fboss/agent/HwSwitchMatcher.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
using std::make_shared;

namespace {
HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}
} // namespace

TEST(SwitchSettingsTest, applyL2LearningConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  *config.switchSettings()->l2LearningMode() = cfg::L2LearningMode::SOFTWARE;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);

  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(
      cfg::L2LearningMode::SOFTWARE, switchSettingsV1->getL2LearningMode());
  EXPECT_FALSE(switchSettingsV1->isQcmEnable());
  EXPECT_FALSE(switchSettingsV1->isPtpTcEnable());
  EXPECT_EQ(300, switchSettingsV1->getL2AgeTimerSeconds());

  *config.switchSettings()->l2LearningMode() = cfg::L2LearningMode::HARDWARE;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);

  auto switchSettingsV2 = util::getFirstNodeIf(stateV2->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV2);
  EXPECT_FALSE(switchSettingsV2->isPublished());
  EXPECT_EQ(
      cfg::L2LearningMode::HARDWARE, switchSettingsV2->getL2LearningMode());
  EXPECT_FALSE(switchSettingsV2->isQcmEnable());
  EXPECT_FALSE(switchSettingsV2->isPtpTcEnable());
  EXPECT_EQ(300, switchSettingsV1->getL2AgeTimerSeconds());
}

TEST(SwitchSettingsTest, applySwitchDrainState) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config = testConfigA(cfg::SwitchType::FABRIC);
  *config.switchSettings()->switchDrainState() = cfg::SwitchDrainState::DRAINED;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);

  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(
      cfg::SwitchDrainState::DRAINED, switchSettingsV1->getSwitchDrainState());

  *config.switchSettings()->switchDrainState() =
      cfg::SwitchDrainState::UNDRAINED;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);

  auto switchSettingsV2 = util::getFirstNodeIf(stateV2->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV2);
  EXPECT_FALSE(switchSettingsV2->isPublished());
  EXPECT_EQ(
      cfg::SwitchDrainState::UNDRAINED,
      switchSettingsV2->getSwitchDrainState());
}

TEST(SwitchSettingsTest, applySwitchDrainOnNonFabricSwitch) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  // VoQ switch config
  cfg::SwitchConfig voqConfig = testConfigA(cfg::SwitchType::VOQ);
  *voqConfig.switchSettings()->switchDrainState() =
      cfg::SwitchDrainState::DRAINED;
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &voqConfig, platform.get()), FbossError);

  // NPU switch config
  cfg::SwitchConfig npuConfig;
  *npuConfig.switchSettings()->switchDrainState() =
      cfg::SwitchDrainState::DRAINED;
  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &npuConfig, platform.get()), FbossError);
}

TEST(SwitchSettingsTest, applyQcmConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  *config.switchSettings()->qcmEnable() = true;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_TRUE(switchSettingsV1->isQcmEnable());
  EXPECT_EQ(
      cfg::L2LearningMode::HARDWARE, switchSettingsV1->getL2LearningMode());
  EXPECT_FALSE(switchSettingsV1->isPtpTcEnable());
  EXPECT_EQ(300, switchSettingsV1->getL2AgeTimerSeconds());

  *config.switchSettings()->qcmEnable() = false;
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto switchSettingsV2 = util::getFirstNodeIf(stateV2->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV2);
  EXPECT_FALSE(switchSettingsV2->isPublished());
  EXPECT_FALSE(switchSettingsV2->isQcmEnable());
  EXPECT_EQ(
      cfg::L2LearningMode::HARDWARE, switchSettingsV2->getL2LearningMode());
  EXPECT_FALSE(switchSettingsV2->isPtpTcEnable());
}

TEST(SwitchSettingsTest, applyPtpTcEnable) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  config.switchSettings()->ptpTcEnable() = true;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_TRUE(switchSettingsV1->isPtpTcEnable());

  config.switchSettings()->ptpTcEnable() = false;
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto switchSettingsV2 = util::getFirstNodeIf(stateV2->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV2);
  EXPECT_FALSE(switchSettingsV2->isPublished());
  EXPECT_FALSE(switchSettingsV2->isPtpTcEnable());
  EXPECT_EQ(300, switchSettingsV1->getL2AgeTimerSeconds());
}

TEST(SwitchSettingsTest, applyL2AgeTimerSeconds) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  addSwitchSettingsToState(stateV0);

  // Check default value
  auto l2AgeTimerSeconds = 300;
  auto switchSettingsV0 = util::getFirstNodeIf(stateV0->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV0);
  EXPECT_FALSE(switchSettingsV0->isPublished());
  EXPECT_EQ(l2AgeTimerSeconds, switchSettingsV0->getL2AgeTimerSeconds());

  // Check if value is updated
  l2AgeTimerSeconds *= 2;
  cfg::SwitchConfig config;
  config.switchSettings()->l2AgeTimerSeconds() = l2AgeTimerSeconds;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(l2AgeTimerSeconds, switchSettingsV1->getL2AgeTimerSeconds());
}

TEST(SwitchSettingsTest, applyMaxRouteCounterIDs) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  addSwitchSettingsToState(stateV0);

  // Check default value
  auto maxRouteCounterIDs = 0;
  auto switchSettingsV0 = util::getFirstNodeIf(stateV0->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV0);
  EXPECT_FALSE(switchSettingsV0->isPublished());
  EXPECT_EQ(maxRouteCounterIDs, switchSettingsV0->getMaxRouteCounterIDs());

  // Check if value is updated
  maxRouteCounterIDs = 10;
  cfg::SwitchConfig config;
  config.switchSettings()->maxRouteCounterIDs() = maxRouteCounterIDs;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(maxRouteCounterIDs, switchSettingsV1->getMaxRouteCounterIDs());
}

TEST(SwitchSettingsTest, applyBlockNeighbors) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  addSwitchSettingsToState(stateV0);

  // Check default value
  auto switchSettingsV0 = util::getFirstNodeIf(stateV0->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV0);
  EXPECT_EQ(switchSettingsV0->getBlockNeighbors()->size(), 0);

  // Check if value is updated
  cfg::SwitchConfig config;

  cfg::Neighbor blockNeighbor;
  blockNeighbor.vlanID() = 1;
  blockNeighbor.ipAddress() = "1.1.1.1";

  config.switchSettings()->blockNeighbors() = {blockNeighbor};
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(switchSettingsV1->getBlockNeighbors()->size(), 1);

  EXPECT_EQ(
      switchSettingsV1->getBlockNeighbors()
          ->at(0)
          ->cref<switch_state_tags::blockNeighborVlanID>()
          ->toThrift(),
      blockNeighbor.vlanID());
  EXPECT_EQ(
      switchSettingsV1->getBlockNeighbors()
          ->at(0)
          ->cref<switch_state_tags::blockNeighborIP>()
          ->toThrift(),
      facebook::network::toBinaryAddress(
          folly::IPAddress(*blockNeighbor.ipAddress())));
}

TEST(SwitchSettingsTest, applyMacAddrsToBlock) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  addSwitchSettingsToState(stateV0);

  // Check default value
  auto switchSettingsV0 = util::getFirstNodeIf(stateV0->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV0);
  EXPECT_EQ(switchSettingsV0->getMacAddrsToBlock()->size(), 0);

  // Check if value is updated
  cfg::SwitchConfig config;

  cfg::MacAndVlan macAddrToBlock;
  macAddrToBlock.vlanID() = 1;
  macAddrToBlock.macAddress() = "00:11:22:33:44:55";

  config.switchSettings()->macAddrsToBlock() = {macAddrToBlock};
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(switchSettingsV1->getMacAddrsToBlock()->size(), 1);

  EXPECT_EQ(
      int(switchSettingsV1->getMacAddrsToBlock_DEPRECATED()[0].first),
      macAddrToBlock.vlanID());
  EXPECT_EQ(
      switchSettingsV1->getMacAddrsToBlock_DEPRECATED()[0].second.toString(),
      macAddrToBlock.macAddress());
}

TEST(SwitchSettingsTest, ThrifyMigration) {
  folly::IPAddress ip("1.1.1.1");
  auto addr = facebook::network::toBinaryAddress(ip);
  std::string jsonStr;
  apache::thrift::SimpleJSONSerializer::serialize(addr, &jsonStr);
  folly::dynamic addrDynamic = folly::parseJson(jsonStr);

  jsonStr = folly::toJson(addrDynamic);
  auto inBuf = folly::IOBuf::wrapBufferAsValue(jsonStr.data(), jsonStr.size());
  auto newAddr = apache::thrift::SimpleJSONSerializer::deserialize<
      facebook::network::thrift::BinaryAddress>(folly::io::Cursor{&inBuf});

  auto newIp = facebook::network::toIPAddress(newAddr);
  EXPECT_EQ(ip, newIp);
  EXPECT_EQ(addr, newAddr);

  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();

  cfg::SwitchConfig config;
  cfg::Neighbor blockNeighbor0;
  blockNeighbor0.vlanID() = 1;
  blockNeighbor0.ipAddress() = "1.1.1.1";
  cfg::Neighbor blockNeighbor1;
  blockNeighbor1.vlanID() = 2;
  blockNeighbor1.ipAddress() = "1::1";
  config.switchSettings()->blockNeighbors() = {blockNeighbor0, blockNeighbor1};

  cfg::MacAndVlan macAddrToBlock;
  macAddrToBlock.vlanID() = 1;
  macAddrToBlock.macAddress() = "00:11:22:33:44:55";
  config.switchSettings()->macAddrsToBlock() = {macAddrToBlock};

  *config.switchSettings()->qcmEnable() = true;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  EXPECT_EQ(*switchSettingsV1->l3SwitchType(), cfg::SwitchType::NPU);
  EXPECT_TRUE(switchSettingsV1->vlansSupported());
  validateThriftMapMapSerialization(*stateV1->getSwitchSettings());
}

TEST(SwitchSettingsTest, applyVoqSwitch) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  addSwitchSettingsToState(
      stateV0, make_shared<SwitchSettings>(), 1 /*switchId*/);

  // Check default value
  auto switchSettingsV0 = util::getFirstNodeIf(stateV0->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV0);
  EXPECT_EQ(switchSettingsV0->getBlockNeighbors()->size(), 0);

  // Check if value is updated
  cfg::SwitchConfig config = testConfigA(cfg::SwitchType::VOQ);
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(switchSettingsV1->getSwitchIds().size(), 1);
  EXPECT_EQ(*switchSettingsV1->getSwitchIds().begin(), SwitchID(1));
  EXPECT_EQ(switchSettingsV1->getSwitchIdToSwitchInfo().size(), 1);
  auto switchInfo = switchSettingsV1->getSwitchIdToSwitchInfo().at(1);
  EXPECT_EQ(switchInfo.switchType(), cfg::SwitchType::VOQ);
  EXPECT_EQ(switchInfo.switchIndex(), 0);
  EXPECT_EQ(
      switchInfo.portIdRange()->minimum(),
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN());
  EXPECT_EQ(
      switchInfo.portIdRange()->maximum(),
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MAX());
  EXPECT_EQ(*switchSettingsV1->l3SwitchType(), cfg::SwitchType::VOQ);
  EXPECT_EQ(
      switchSettingsV1->getSwitchIdsOfType(cfg::SwitchType::VOQ).size(), 1);
  EXPECT_EQ(
      switchSettingsV1->getSwitchIdsOfType(cfg::SwitchType::NPU).size(), 0);
  EXPECT_FALSE(switchSettingsV1->vlansSupported());
  EXPECT_EQ(switchInfo.asicType(), cfg::AsicType::ASIC_TYPE_MOCK);
  EXPECT_THROW(switchSettingsV1->getSwitchType(0), FbossError);
  cfg::SwitchInfo switchInfo2;
  switchInfo2.switchType() = cfg::SwitchType::FABRIC;
  switchInfo2.asicType() = cfg::AsicType::ASIC_TYPE_MOCK;
  cfg::Range64 portIdRange;
  portIdRange.minimum() =
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN();
  portIdRange.maximum() =
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MAX();
  switchInfo2.portIdRange() = portIdRange;
  config.switchSettings()->switchIdToSwitchInfo() = {
      std::make_pair(2, switchInfo2)};
  EXPECT_THROW(
      publishAndApplyConfig(stateV1, &config, platform.get()), FbossError);
  // Should allow range and switchIndex changes
  // TODO - block this after config is rolled out everywhere
  switchInfo2.switchType() = cfg::SwitchType::VOQ;
  cfg::Range64 portIdRange2;
  portIdRange2.minimum() =
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN();
  portIdRange2.maximum() =
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MAX() + 1;
  switchInfo2.portIdRange() = portIdRange2;
  cfg::Range64 sysPortRange;
  sysPortRange.minimum() = 100;
  sysPortRange.maximum() = 200;
  switchInfo2.systemPortRange() = sysPortRange;
  config.switchSettings()->switchIdToSwitchInfo() = {
      std::make_pair(1, switchInfo2)};
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto switchSettingsV2 = util::getFirstNodeIf(stateV2->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV2);
  auto switchInfo3 = switchSettingsV2->getSwitchIdToSwitchInfo().at(1);
  EXPECT_EQ(
      switchInfo3.portIdRange()->minimum(),
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MIN());
  EXPECT_EQ(
      switchInfo3.portIdRange()->maximum(),
      cfg::switch_config_constants::DEFAULT_PORT_ID_RANGE_MAX() + 1);
  validateNodeSerialization(*switchSettingsV1);
}

TEST(SwitchSettingsTest, applyExactMatchTableConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  addSwitchSettingsToState(stateV0);

  // Check default value
  auto switchSettingsV0 = util::getFirstNodeIf(stateV0->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV0);
  EXPECT_EQ(switchSettingsV0->getExactMatchTableConfig()->size(), 0);

  // Check if value is updated
  cfg::SwitchConfig config;

  cfg::ExactMatchTableConfig tableConfig;
  tableConfig.name() = "TeFlowTable";
  tableConfig.dstPrefixLength() = 59;

  config.switchSettings()->exactMatchTableConfigs() = {tableConfig};
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(switchSettingsV1->getExactMatchTableConfig()->size(), 1);
  EXPECT_EQ(
      switchSettingsV1->getExactMatchTableConfig()
          ->at(0)
          ->cref<switch_config_tags::name>()
          ->toThrift(),
      "TeFlowTable");

  EXPECT_EQ(
      switchSettingsV1->getExactMatchTableConfig()
          ->at(0)
          ->cref<switch_config_tags::dstPrefixLength>()
          ->toThrift(),
      59);

  // update prefix length
  (*config.switchSettings()->exactMatchTableConfigs())[0].dstPrefixLength() =
      51;
  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_EQ(
      util::getFirstNodeIf(stateV2->getSwitchSettings())
          ->getExactMatchTableConfig()
          ->at(0)
          ->cref<switch_config_tags::dstPrefixLength>()
          ->toThrift(),
      51);

  // delete the config
  cfg::SwitchConfig emptyConfig;
  auto stateV3 = publishAndApplyConfig(stateV2, &emptyConfig, platform.get());
  EXPECT_NE(nullptr, stateV3);
  EXPECT_EQ(
      util::getFirstNodeIf(stateV3->getSwitchSettings())
          ->getExactMatchTableConfig()
          ->size(),
      0);
}

TEST(SwitchSettingsTest, applyDefaultVlanConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  addSwitchSettingsToState(stateV0);

  // Check default value
  auto switchSettingsV0 = util::getFirstNodeIf(stateV0->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV0);
  EXPECT_EQ(switchSettingsV0->getDefaultVlan(), std::nullopt);

  // Check whether value is updated
  cfg::SwitchConfig config = testConfigA();
  config.defaultVlan() = 1;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(switchSettingsV1->getDefaultVlan(), 1);

  const auto& thriftState0 = stateV1->toThrift();
  EXPECT_EQ(thriftState0.defaultVlan(), 1);
  EXPECT_EQ(thriftState0.switchSettings()->defaultVlan(), 1);
}

TEST(SwitchSettingsTest, applyArpNdpTimeoutConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  addSwitchSettingsToState(stateV0);

  // Check default value
  auto switchSettingsV0 = util::getFirstNodeIf(stateV0->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV0);
  EXPECT_EQ(switchSettingsV0->getArpTimeout(), std::nullopt);
  EXPECT_EQ(switchSettingsV0->getNdpTimeout(), std::nullopt);
  EXPECT_EQ(switchSettingsV0->getArpAgerInterval(), std::nullopt);
  EXPECT_EQ(switchSettingsV0->getStaleEntryInterval(), std::nullopt);
  EXPECT_EQ(switchSettingsV0->getMaxNeighborProbes(), std::nullopt);

  // Check whether value is updated
  cfg::SwitchConfig config = testConfigA();
  config.arpTimeoutSeconds() = 300;
  config.arpAgerInterval() = 200;
  config.staleEntryInterval() = 200;
  config.maxNeighborProbes() = 100;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(switchSettingsV1->getArpTimeout(), std::chrono::seconds(300));
  EXPECT_EQ(switchSettingsV1->getNdpTimeout(), std::chrono::seconds(300));
  EXPECT_EQ(switchSettingsV1->getArpAgerInterval(), std::chrono::seconds(200));
  EXPECT_EQ(
      switchSettingsV1->getStaleEntryInterval(), std::chrono::seconds(200));
  EXPECT_EQ(switchSettingsV1->getMaxNeighborProbes(), 100);

  const auto& thriftState0 = stateV1->toThrift();
  EXPECT_EQ(thriftState0.arpTimeout(), 300);
  EXPECT_EQ(thriftState0.ndpTimeout(), 300);
  EXPECT_EQ(thriftState0.arpAgerInterval(), 200);
  EXPECT_EQ(thriftState0.staleEntryInterval(), 200);
  EXPECT_EQ(thriftState0.maxNeighborProbes(), 100);
  EXPECT_EQ(thriftState0.switchSettings()->arpTimeout(), 300);
  EXPECT_EQ(thriftState0.switchSettings()->ndpTimeout(), 300);
  EXPECT_EQ(thriftState0.switchSettings()->arpAgerInterval(), 200);
  EXPECT_EQ(thriftState0.switchSettings()->staleEntryInterval(), 200);
  EXPECT_EQ(thriftState0.switchSettings()->maxNeighborProbes(), 100);
}

TEST(SwitchSettingsTest, applyDhcpConfig) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  addSwitchSettingsToState(stateV0);

  const folly::IPAddressV6 kDhcpV6RelaySrc("100::1");
  const folly::IPAddressV6 kDhcpV6ReplySrc("101::1");
  const folly::IPAddressV4 kDhcpV4RelaySrc("100.0.0.1");
  const folly::IPAddressV4 kDhcpV4ReplySrc("101.0.0.1");

  // Check default value
  auto switchSettingsV0 = util::getFirstNodeIf(stateV0->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV0);
  EXPECT_EQ(switchSettingsV0->getDhcpV4RelaySrc(), std::nullopt);
  EXPECT_EQ(switchSettingsV0->getDhcpV6RelaySrc(), std::nullopt);
  EXPECT_EQ(switchSettingsV0->getDhcpV4ReplySrc(), std::nullopt);
  EXPECT_EQ(switchSettingsV0->getDhcpV6ReplySrc(), std::nullopt);

  // Check whether value is updated
  cfg::SwitchConfig config = testConfigA();
  config.dhcpRelaySrcOverrideV4() = "100.0.0.1";
  config.dhcpReplySrcOverrideV4() = "101.0.0.1";
  config.dhcpRelaySrcOverrideV6() = "100::1";
  config.dhcpReplySrcOverrideV6() = "101::1";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto switchSettingsV1 = util::getFirstNodeIf(stateV1->getSwitchSettings());
  ASSERT_NE(nullptr, switchSettingsV1);
  EXPECT_FALSE(switchSettingsV1->isPublished());
  EXPECT_EQ(switchSettingsV1->getDhcpV4RelaySrc(), kDhcpV4RelaySrc);
  EXPECT_EQ(switchSettingsV1->getDhcpV6RelaySrc(), kDhcpV6RelaySrc);
  EXPECT_EQ(switchSettingsV1->getDhcpV4ReplySrc(), kDhcpV4ReplySrc);
  EXPECT_EQ(switchSettingsV1->getDhcpV6ReplySrc(), kDhcpV6ReplySrc);

  const auto& thriftState0 = stateV1->toThrift();
  EXPECT_EQ(
      thriftState0.dhcpV4RelaySrc(),
      facebook::network::toBinaryAddress(kDhcpV4RelaySrc));
  EXPECT_EQ(
      thriftState0.dhcpV6RelaySrc(),
      facebook::network::toBinaryAddress(kDhcpV6RelaySrc));
  EXPECT_EQ(
      thriftState0.dhcpV4ReplySrc(),
      facebook::network::toBinaryAddress(kDhcpV4ReplySrc));
  EXPECT_EQ(
      thriftState0.dhcpV6ReplySrc(),
      facebook::network::toBinaryAddress(kDhcpV6ReplySrc));
  EXPECT_EQ(
      thriftState0.switchSettings()->dhcpV4RelaySrc(),
      facebook::network::toBinaryAddress(kDhcpV4RelaySrc));
  EXPECT_EQ(
      thriftState0.switchSettings()->dhcpV6RelaySrc(),
      facebook::network::toBinaryAddress(kDhcpV6RelaySrc));
  EXPECT_EQ(
      thriftState0.switchSettings()->dhcpV4ReplySrc(),
      facebook::network::toBinaryAddress(kDhcpV4ReplySrc));
  EXPECT_EQ(
      thriftState0.switchSettings()->dhcpV6ReplySrc(),
      facebook::network::toBinaryAddress(kDhcpV6ReplySrc));
}

TEST(SwitchSettingsTest, modify) {
  auto stateV0 = make_shared<SwitchState>();
  auto multiSwitchSwitchSettingsV0 = make_shared<MultiSwitchSettings>();
  auto switchSettingsV0 = make_shared<SwitchSettings>();
  multiSwitchSwitchSettingsV0->addNode(
      scope().matcherString(), switchSettingsV0);
  stateV0->resetSwitchSettings(multiSwitchSwitchSettingsV0);
  EXPECT_EQ(switchSettingsV0.get(), switchSettingsV0->modify(&stateV0));
  stateV0->publish();
  auto newSwitchSettingsV0 = switchSettingsV0->modify(&stateV0);
  EXPECT_NE(newSwitchSettingsV0, switchSettingsV0.get());
  EXPECT_NE(
      stateV0->getSwitchSettings().get(), multiSwitchSwitchSettingsV0.get());
}
