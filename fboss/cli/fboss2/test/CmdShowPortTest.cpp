// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/commands/show/port/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

#include "configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

using namespace ::testing;
using namespace facebook::neteng::fboss::bgp::thrift;

namespace facebook::fboss {

/*
 * Set up port test data
 */
std::map<int32_t, PortInfoThrift> createPortEntries() {
  std::map<int32_t, PortInfoThrift> portMap;

  PortInfoThrift portEntry1;
  portEntry1.portId() = 1;
  portEntry1.name() = "eth1/5/1";
  portEntry1.adminState() = PortAdminState::ENABLED;
  portEntry1.operState() = PortOperState::DOWN;
  portEntry1.speedMbps() = 100000;
  portEntry1.profileID() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  portEntry1.hwLogicalPortId() = 1;
  TransceiverIdxThrift tcvr1;
  tcvr1.transceiverId() = 0;
  portEntry1.transceiverIdx() = tcvr1;
  PfcConfig pfcCfg;
  pfcCfg.tx() = true;
  pfcCfg.rx() = true;
  pfcCfg.watchdog() = true;
  portEntry1.pfc() = pfcCfg;
  portEntry1.isDrained() = false;
  portEntry1.coreId() = 1;
  portEntry1.virtualDeviceId() = 1;

  PortInfoThrift portEntry2;
  portEntry2.portId() = 2;
  portEntry2.name() = "eth1/5/2";
  portEntry2.adminState() = PortAdminState::DISABLED;
  portEntry2.operState() = PortOperState::DOWN;
  portEntry2.speedMbps() = 25000;
  portEntry2.profileID() = "PROFILE_25G_1_NRZ_CL74_COPPER";
  portEntry2.hwLogicalPortId() = 2;
  TransceiverIdxThrift tcvr2;
  tcvr2.transceiverId() = 1;
  portEntry2.transceiverIdx() = tcvr2;
  portEntry2.rxPause() = true;
  portEntry2.isDrained() = false;
  portEntry2.coreId() = 2;
  portEntry2.virtualDeviceId() = 2;

  PortInfoThrift portEntry3;
  portEntry3.portId() = 3;
  portEntry3.name() = "eth1/5/3";
  portEntry3.adminState() = PortAdminState::ENABLED;
  portEntry3.operState() = PortOperState::UP;
  portEntry3.speedMbps() = 100000;
  portEntry3.profileID() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  portEntry3.hwLogicalPortId() = 3;
  TransceiverIdxThrift tcvr3;
  tcvr3.transceiverId() = 2;
  portEntry3.transceiverIdx() = tcvr3;
  portEntry3.isDrained() = false;
  portEntry3.coreId() = 3;
  portEntry3.virtualDeviceId() = 3;

  PortInfoThrift portEntry4;
  portEntry4.portId() = 8;
  portEntry4.name() = "fab402/9/1";
  portEntry4.adminState() = PortAdminState::ENABLED;
  portEntry4.operState() = PortOperState::UP;
  portEntry4.speedMbps() = 100000;
  portEntry4.profileID() = "PROFILE_100G_4_NRZ_NOFEC_COPPER";
  portEntry4.hwLogicalPortId() = 8;
  TransceiverIdxThrift tcvr4;
  tcvr4.transceiverId() = 3;
  portEntry4.transceiverIdx() = tcvr4;
  portEntry4.isDrained() = true;

  PortInfoThrift portEntry5;
  portEntry5.portId() = 7;
  portEntry5.name() = "eth1/10/2";
  portEntry5.adminState() = PortAdminState::ENABLED;
  portEntry5.operState() = PortOperState::UP;
  portEntry5.speedMbps() = 100000;
  portEntry5.profileID() = "PROFILE_100G_4_NRZ_CL91_OPTICAL";
  portEntry5.hwLogicalPortId() = 7;
  TransceiverIdxThrift tcvr5;
  tcvr5.transceiverId() = 4;
  portEntry5.transceiverIdx() = tcvr5;
  portEntry5.isDrained() = false;
  portEntry5.coreId() = 5;
  portEntry5.virtualDeviceId() = 5;

  PortInfoThrift portEntry6;
  portEntry6.portId() = 9;
  portEntry6.name() = "eth1/4/1";
  portEntry6.adminState() = PortAdminState::ENABLED;
  portEntry6.operState() = PortOperState::UP;
  portEntry6.speedMbps() = 100000;
  portEntry6.profileID() = "PROFILE_100G_4_NRZ_CL91_OPTICAL";
  portEntry6.hwLogicalPortId() = 9;
  TransceiverIdxThrift tcvr6;
  tcvr6.transceiverId() = 5;
  portEntry6.transceiverIdx() = tcvr6;
  portEntry6.isDrained() = false;
  portEntry6.coreId() = 6;
  portEntry6.virtualDeviceId() = 6;

  portMap[portEntry1.get_portId()] = portEntry1;
  portMap[portEntry2.get_portId()] = portEntry2;
  portMap[portEntry3.get_portId()] = portEntry3;
  portMap[portEntry4.get_portId()] = portEntry4;
  portMap[portEntry5.get_portId()] = portEntry5;
  portMap[portEntry6.get_portId()] = portEntry6;
  return portMap;
}

std::map<int32_t, PortInfoThrift> createInvalidPortEntries() {
  std::map<int32_t, PortInfoThrift> portMap;

  // port name format <module name><module number>/<port number>/<subport
  // number>
  // missing <module number>

  PortInfoThrift portEntry1, portEntry2;
  portEntry1.portId() = 1;
  portEntry1.name() = "eth/5/1";

  portEntry2.portId() = 2;
  portEntry2.name() = "eth/5/1";

  portMap[portEntry1.get_portId()] = portEntry1;
  portMap[portEntry2.get_portId()] = portEntry2;
  return portMap;
}

/*
 * Set up transceiver test data
 */
std::map<int, TransceiverInfo> createTransceiverEntries() {
  std::map<int, TransceiverInfo> transceiverMap;

  TransceiverInfo transceiverEntry1;
  transceiverEntry1.tcvrState()->present() = true;

  TransceiverInfo transceiverEntry2;
  transceiverEntry2.tcvrState()->present() = true;

  TransceiverInfo transceiverEntry3;
  transceiverEntry3.tcvrState()->present() = false;

  TransceiverInfo transceiverEntry4;
  transceiverEntry4.tcvrState()->present() = false;

  TransceiverInfo transceiverEntry5;
  transceiverEntry5.tcvrState()->present() = true;

  TransceiverInfo transceiverEntry6;
  transceiverEntry6.tcvrState()->present() = true;

  transceiverMap[0] = transceiverEntry1;
  transceiverMap[1] = transceiverEntry2;
  transceiverMap[2] = transceiverEntry3;
  transceiverMap[3] = transceiverEntry4;
  transceiverMap[4] = transceiverEntry5;
  transceiverMap[5] = transceiverEntry6;
  return transceiverMap;
}

cli::ShowPortModel createPortModel() {
  cli::ShowPortModel model;

  cli::PortEntry entry1, entry2, entry3, entry4, entry5, entry6;
  entry1.id() = 1;
  entry1.hwLogicalPortId() = 1;
  entry1.name() = "eth1/5/1";
  entry1.adminState() = "Enabled";
  entry1.linkState() = "Down";
  entry1.activeState() = "--";
  entry1.speed() = "100G";
  entry1.profileId() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  entry1.tcvrID() = 0;
  entry1.tcvrPresent() = "Present";
  entry1.numUnicastQueues() = 0;
  // when pfc exists, pause shouldn't
  entry1.pfc() = "TX RX WD";
  entry1.isDrained() = "Yes";
  entry1.activeErrors() = "--";
  entry1.coreId() = "1";
  entry1.virtualDeviceId() = "1";

  entry2.id() = 2;
  entry2.hwLogicalPortId() = 2;
  entry2.name() = "eth1/5/2";
  entry2.adminState() = "Disabled";
  entry2.linkState() = "Down";
  entry2.activeState() = "--";
  entry2.speed() = "25G";
  entry2.profileId() = "PROFILE_25G_1_NRZ_CL74_COPPER";
  entry2.tcvrID() = 1;
  entry2.tcvrPresent() = "Present";
  entry2.numUnicastQueues() = 0;
  entry2.pause() = "RX";
  entry2.isDrained() = "No";
  entry2.activeErrors() = "--";
  entry2.coreId() = "2";
  entry2.virtualDeviceId() = "2";

  entry3.id() = 3;
  entry3.hwLogicalPortId() = 3;
  entry3.name() = "eth1/5/3";
  entry3.adminState() = "Enabled";
  entry3.linkState() = "Up";
  entry3.activeState() = "--";
  entry3.speed() = "100G";
  entry3.profileId() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  entry3.tcvrID() = 2;
  entry3.tcvrPresent() = "Absent";
  entry3.numUnicastQueues() = 0;
  entry3.pause() = "";
  entry3.isDrained() = "No";
  entry3.activeErrors() = "--";
  entry3.coreId() = "3";
  entry3.virtualDeviceId() = "3";

  entry4.id() = 8;
  entry4.hwLogicalPortId() = 8;
  entry4.name() = "fab402/9/1";
  entry4.adminState() = "Enabled";
  entry4.linkState() = "Up";
  entry4.activeState() = "--";
  entry4.speed() = "100G";
  entry4.profileId() = "PROFILE_100G_4_NRZ_NOFEC_COPPER";
  entry4.tcvrID() = 3;
  entry4.tcvrPresent() = "Absent";
  entry4.numUnicastQueues() = 0;
  entry4.pause() = "";
  entry4.isDrained() = "Yes";
  entry4.activeErrors() = "--";
  entry4.coreId() = "--";
  entry4.virtualDeviceId() = "--";

  entry5.id() = 7;
  entry5.hwLogicalPortId() = 7;
  entry5.name() = "eth1/10/2";
  entry5.adminState() = "Enabled";
  entry5.linkState() = "Up";
  entry5.activeState() = "--";
  entry5.speed() = "100G";
  entry5.profileId() = "PROFILE_100G_4_NRZ_CL91_OPTICAL";
  entry5.tcvrID() = 4;
  entry5.tcvrPresent() = "Present";
  entry5.numUnicastQueues() = 0;
  entry5.pause() = "";
  entry5.isDrained() = "No";
  entry5.activeErrors() = "--";
  entry5.coreId() = "5";
  entry5.virtualDeviceId() = "5";

  entry6.id() = 9;
  entry6.hwLogicalPortId() = 9;
  entry6.name() = "eth1/4/1";
  entry6.adminState() = "Enabled";
  entry6.linkState() = "Up";
  entry6.activeState() = "--";
  entry6.speed() = "100G";
  entry6.profileId() = "PROFILE_100G_4_NRZ_CL91_OPTICAL";
  entry6.tcvrID() = 5;
  entry6.tcvrPresent() = "Present";
  entry6.numUnicastQueues() = 0;
  entry6.pause() = "";
  entry6.isDrained() = "Yes";
  entry6.activeErrors() = "--";
  entry6.coreId() = "6";
  entry6.virtualDeviceId() = "6";

  // sorted by name
  model.portEntries() = {entry6, entry1, entry2, entry3, entry5, entry4};
  return model;
}

std::vector<std::string> createDrainedInterfaces() {
  std::vector<std::string> drainedInterfaces{"eth1/4/1", "eth1/5/1"};
  return drainedInterfaces;
}

std::string createMockedBgpConfig() {
  std::string bgpConfigStr;
  bgp::thrift::BgpConfig bgpConfig;
  std::vector<std::string> drainedInterfaces = createDrainedInterfaces();
  bgpConfig.drained_interfaces() = std::move(drainedInterfaces);
  apache::thrift::SimpleJSONSerializer::serialize(bgpConfig, &bgpConfigStr);
  return bgpConfigStr;
}

class CmdShowPortTestFixture : public CmdHandlerTestBase {
 public:
  CmdShowPortTraits::ObjectArgType queriedEntries;
  std::map<int32_t, facebook::fboss::PortInfoThrift> mockPortEntries;
  std::map<int32_t, facebook::fboss::TransceiverInfo> mockTransceiverEntries;
  std::map<std::string, facebook::fboss::HwPortStats> mockPortStats;
  cli::ShowPortModel normalizedModel;
  std::vector<std::string> mockDrainedInterfaces;
  std::string mockBgpRunningConfig;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    mockPortEntries = createPortEntries();
    mockTransceiverEntries = createTransceiverEntries();
    normalizedModel = createPortModel();
    mockDrainedInterfaces = createDrainedInterfaces();
    mockBgpRunningConfig = createMockedBgpConfig();
  }
};

TEST_F(CmdShowPortTestFixture, sortByName) {
  auto model = CmdShowPort().createModel(
      mockPortEntries,
      mockTransceiverEntries,
      queriedEntries,
      mockPortStats,
      mockDrainedInterfaces);

  EXPECT_THRIFT_EQ(model, normalizedModel);
}

TEST_F(CmdShowPortTestFixture, invalidPortName) {
  auto invalidPortEntries = createInvalidPortEntries();

  try {
    CmdShowPort().createModel(
        invalidPortEntries,
        mockTransceiverEntries,
        queriedEntries,
        mockPortStats,
        mockDrainedInterfaces);
    FAIL();
  } catch (const std::invalid_argument& expected) {
    ASSERT_STREQ(
        "Invalid port name: eth/5/1\n"
        "Port name must match 'moduleNum/port/subport' pattern",
        expected.what());
  }
}

TEST_F(CmdShowPortTestFixture, queryClient) {
  setupMockedAgentServer();
  setupMockedBgpServer();
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = mockPortEntries; }));

  EXPECT_CALL(getQsfpService(), getTransceiverInfo(_, _))
      .WillOnce(Invoke(
          [&](auto& entries, auto) { entries = mockTransceiverEntries; }));

  EXPECT_CALL(getBgpService(), getRunningConfig(_))
      .WillOnce(Invoke(
          [&](auto& bgpConfigStr) { bgpConfigStr = mockBgpRunningConfig; }));

  auto cmd = CmdShowPort();
  CmdShowPortTraits::ObjectArgType queriedEntries;
  auto model = cmd.queryClient(localhost(), queriedEntries);

  EXPECT_THRIFT_EQ(model, normalizedModel);
}

} // namespace facebook::fboss
