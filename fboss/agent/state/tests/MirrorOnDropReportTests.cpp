// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/hw/mock/MockPlatformMapping.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

// see TestUtils > testConfigAImpl()
constexpr int kRecyclePortId = 1;
constexpr int kGlobalScopedRecyclePortId = 2;
constexpr int kNifPortId = 5;

class MirrorOnDropReportTest : public ::testing::Test {
 protected:
  void SetUp() override {
    platform_ = createMockPlatform(cfg::SwitchType::VOQ, kVoqSwitchIdBegin);
    state_ = std::make_shared<SwitchState>();
    addSwitchInfo(
        state_,
        cfg::SwitchType::VOQ,
        kVoqSwitchIdBegin /* switchId */,
        cfg::AsicType::ASIC_TYPE_JERICHO3);
    config_ =
        testConfigA(cfg::SwitchType::VOQ, cfg::AsicType::ASIC_TYPE_JERICHO3);

    // Add a globally scoped recycle port
    auto platformPort =
        mockPlatformMapping_.getPlatformPort(kGlobalScopedRecyclePortId);
    platformPort.mapping()->scope() = cfg::Scope::GLOBAL;
    mockPlatformMapping_.setPlatformPort(
        kGlobalScopedRecyclePortId, platformPort);

    cfg::Port recyclePort;
    recyclePort.logicalID() = kGlobalScopedRecyclePortId;
    recyclePort.name() = "rcy1/1/2";
    recyclePort.speed() = cfg::PortSpeed::XG;
    recyclePort.profileID() =
        cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_COPPER;
    recyclePort.portType() = cfg::PortType::RECYCLE_PORT;
    recyclePort.scope() = cfg::Scope::GLOBAL;
    config_.ports()->push_back(recyclePort);
  }

  std::shared_ptr<SwitchState> state_;
  std::shared_ptr<Platform> platform_;
  cfg::SwitchConfig config_;
  MockPlatformMapping mockPlatformMapping_;
};

cfg::MirrorOnDropEventConfig makeEventConfig(
    std::optional<cfg::MirrorOnDropAgingGroup> group,
    const std::vector<cfg::MirrorOnDropReasonAggregation>& reasonAggs) {
  cfg::MirrorOnDropEventConfig eventCfg;
  if (group.has_value()) {
    eventCfg.agingGroup() = group.value();
  }
  eventCfg.dropReasonAggregations() = reasonAggs;
  return eventCfg;
}

cfg::MirrorOnDropReport makeReportCfg(int portId, const std::string& ip) {
  cfg::MirrorOnDropReport report;
  report.name() = "mod-1";
  report.mirrorPortId() = portId;
  report.localSrcPort() = 10000;
  report.collectorIp() = ip;
  report.collectorPort() = 20000;
  report.mtu() = 1500;
  report.truncateSize() = 128;
  report.dscp() = 0;
  report.modEventToConfigMap()[0] = makeEventConfig(
      std::nullopt,
      {cfg::MirrorOnDropReasonAggregation::INGRESS_PACKET_PROCESSING_DISCARDS});
  report.modEventToConfigMap()[1] = makeEventConfig(
      cfg::MirrorOnDropAgingGroup::PORT,
      {
          cfg::MirrorOnDropReasonAggregation::
              INGRESS_SOURCE_CONGESTION_DISCARDS,
          cfg::MirrorOnDropReasonAggregation::
              INGRESS_DESTINATION_CONGESTION_DISCARDS,
      });
  report.agingGroupAgingIntervalUsecs()[cfg::MirrorOnDropAgingGroup::PORT] =
      100;
  return report;
}

TEST_F(MirrorOnDropReportTest, CreateReportV4) {
  config_.mirrorOnDropReports()->push_back(
      makeReportCfg(kRecyclePortId, "1.2.3.4"));

  state_ = publishAndApplyConfig(
      state_, &config_, platform_.get(), nullptr, &mockPlatformMapping_);

  auto report = state_->getMirrorOnDropReports()->getNodeIf("mod-1");
  EXPECT_NE(report, nullptr);
  EXPECT_EQ(report->getID(), "mod-1");
  EXPECT_TRUE(report->getLocalSrcIp().isV4());
  EXPECT_FALSE(report->getSwitchMac().empty());
  EXPECT_FALSE(report->getFirstInterfaceMac().empty());
  EXPECT_EQ(report->getModEventToConfigMap().size(), 2);
  EXPECT_EQ(
      report->getModEventToConfigMap()[0].agingGroup().value_or(
          cfg::MirrorOnDropAgingGroup::GLOBAL),
      cfg::MirrorOnDropAgingGroup::GLOBAL);
  EXPECT_EQ(
      report->getModEventToConfigMap()[1].agingGroup().value_or(
          cfg::MirrorOnDropAgingGroup::GLOBAL),
      cfg::MirrorOnDropAgingGroup::PORT);
  EXPECT_EQ(
      report->getAgingGroupAgingIntervalUsecs()
          [cfg::MirrorOnDropAgingGroup::PORT],
      100);
}

TEST_F(MirrorOnDropReportTest, CreateReportV6) {
  config_.mirrorOnDropReports()->push_back(
      makeReportCfg(kRecyclePortId, "2401::1"));

  state_ = publishAndApplyConfig(
      state_, &config_, platform_.get(), nullptr, &mockPlatformMapping_);

  auto report = state_->getMirrorOnDropReports()->getNodeIf("mod-1");
  EXPECT_NE(report, nullptr);
  EXPECT_EQ(report->getID(), "mod-1");
  EXPECT_TRUE(report->getLocalSrcIp().isV6());
  EXPECT_FALSE(report->getSwitchMac().empty());
  EXPECT_FALSE(report->getFirstInterfaceMac().empty());
  EXPECT_EQ(report->getModEventToConfigMap().size(), 2);
  EXPECT_EQ(
      report->getModEventToConfigMap()[0].agingGroup().value_or(
          cfg::MirrorOnDropAgingGroup::GLOBAL),
      cfg::MirrorOnDropAgingGroup::GLOBAL);
  EXPECT_EQ(
      report->getModEventToConfigMap()[1].agingGroup().value_or(
          cfg::MirrorOnDropAgingGroup::GLOBAL),
      cfg::MirrorOnDropAgingGroup::PORT);
  EXPECT_EQ(
      report->getAgingGroupAgingIntervalUsecs()
          [cfg::MirrorOnDropAgingGroup::PORT],
      100);
}

TEST_F(MirrorOnDropReportTest, CreateReportOnNifPort) {
  config_.mirrorOnDropReports()->push_back(
      makeReportCfg(kNifPortId, "2401::1"));
  EXPECT_THROW(
      publishAndApplyConfig(
          state_, &config_, platform_.get(), nullptr, &mockPlatformMapping_),
      FbossError);
}

TEST_F(MirrorOnDropReportTest, CreateReportOnGlobalScopedRecyclePort) {
  config_.mirrorOnDropReports()->push_back(
      makeReportCfg(kGlobalScopedRecyclePortId, "2401::1"));
  EXPECT_THROW(
      publishAndApplyConfig(
          state_, &config_, platform_.get(), nullptr, &mockPlatformMapping_),
      FbossError);
}

} // namespace facebook::fboss
