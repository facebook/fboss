// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

class MirrorOnDropReportTest : public ::testing::Test {
 protected:
  void SetUp() override {
    platform_ = createMockPlatform(cfg::SwitchType::VOQ, kVoqSwitchIdBegin);
    state_ = std::make_shared<SwitchState>();
    addSwitchInfo(
        state_, cfg::SwitchType::VOQ, kVoqSwitchIdBegin /* switchId */);
    config_ = testConfigA(cfg::SwitchType::VOQ);
  }

  std::shared_ptr<SwitchState> state_;
  std::shared_ptr<Platform> platform_;
  cfg::SwitchConfig config_;
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

cfg::MirrorOnDropReport makeReportCfg(const std::string& ip) {
  cfg::MirrorOnDropReport report;
  report.name() = "mod-1";
  report.mirrorPortId() = 5;
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
  config_.mirrorOnDropReports()->push_back(makeReportCfg("1.2.3.4"));

  state_ = publishAndApplyConfig(state_, &config_, platform_.get());

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
  config_.mirrorOnDropReports()->push_back(makeReportCfg("2401::1"));

  state_ = publishAndApplyConfig(state_, &config_, platform_.get());

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

} // namespace facebook::fboss
