// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>

#include <folly/Conv.h>
#include <folly/MacAddress.h>
#include <folly/String.h>
#include <folly/io/Cursor.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/agent_hw_tests/AgentMPLSDataplaneTestUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PortStatsTestUtils.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

template <typename PortType>
class AgentMPLSDataplaneTest : public AgentHwTest {
 protected:
  static constexpr bool kIsTrunk = std::is_same_v<PortType, AggregatePortID>;
  static constexpr auto kGetQueueOutPktsRetryTimes = 5;

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_observe_rx_packets_without_interface = true;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /* interfaceHasSubnet */);

    if constexpr (kIsTrunk) {
      const auto memberPort = ensemble.masterLogicalPortIds()[0];
      utility::addAggPort(1, {memberPort}, &config);

      // P200/Chenab: a PORT RIF on a LAG member collides with LAG eth-port
      // creation (SAI create_ethernet_port -> OBJECT IN USE). Route via the
      // default VLAN SVI instead; EcmpSetupHelper maps AggregatePortID to the
      // member VLAN and looks up InterfaceID == vlanID.
      auto& interfaces = *config.interfaces();
      interfaces.erase(
          std::remove_if(
              interfaces.begin(),
              interfaces.end(),
              [&](const cfg::Interface& intf) {
                return intf.type() == cfg::InterfaceType::PORT &&
                    intf.portID().has_value() &&
                    PortID(*intf.portID()) == memberPort;
              }),
          interfaces.end());

      if (config.defaultVlan().has_value()) {
        const auto defaultVlanId = *config.defaultVlan();
        for (auto& intf : *config.interfaces()) {
          if (intf.name().has_value() && *intf.name() == "default_vlan_rif") {
            intf.intfID() = defaultVlanId;
            break;
          }
        }
        // Keep vlan->interfaceID consistent with default_vlan_rif (ConfigUtils
        // initially sets intfID 10 on both; EcmpSetupHelper looks up interface
        // by vlan ID, getNeighborTableForVlan uses vlan.getInterfaceID()).
        for (auto& vlan : *config.vlans()) {
          if (vlan.id().has_value() && *vlan.id() == defaultVlanId) {
            vlan.intfID() = defaultVlanId;
            break;
          }
        }
      }
    }

    utility::setDefaultCpuTrafficPolicyConfig(
        config, ensemble.getL3Asics(), ensemble.isSai());
    utility::addCpuQueueConfig(config, ensemble.getL3Asics(), ensemble.isSai());
    return config;
  }

  PortID egressPort() const {
    return this->masterLogicalInterfacePortIds()[0];
  }

  PortDescriptor egressPortDescriptor() const {
    if constexpr (kIsTrunk) {
      return PortDescriptor(AggregatePortID(1));
    }
    return PortDescriptor(egressPort());
  }

  PortID ingressPort() const {
    return this->masterLogicalInterfacePortIds()[1];
  }

  PortID secondPassEgressPort() const {
    const auto& ports = this->masterLogicalInterfacePortIds();
    CHECK_GE(ports.size(), 3)
        << "MPLS dataplane TTL-expiry tests require at least 3 interface "
           "ports (egress, ingress inject, second-pass egress). Use a "
           "platform mapping with >= 3 ports, e.g. "
           "Wedge400CPlatformMapping-16port.json";
    return ports[2];
  }

  folly::MacAddress routerMac() const {
    return getMacForFirstInterfaceWithPortsForTesting(
        this->getProgrammedState());
  }

  void applyConfigAndEnableTrunks(const cfg::SwitchConfig& config) {
    this->applyNewConfig(config);
    if constexpr (kIsTrunk) {
      this->applyNewState(
          [](const std::shared_ptr<SwitchState>& state) {
            return utility::enableTrunkPorts(state);
          },
          "enable trunk ports");
    }
  }

  Label pushedTopLabel(
      const LabelForwardingAction::LabelStack& pushStack) const {
    CHECK(!pushStack.empty());
    return pushStack.back();
  }

  LabelForwardingAction::LabelStack pushedLabelStack(
      uint32_t baseLabel,
      uint32_t count) const {
    CHECK_GT(count, 0);
    return utility::mpls_dataplane_test::makeLabelStack(baseLabel, count);
  }

  LabelForwardingAction::LabelStack maxPushedLabelStack(
      uint32_t baseLabel) const {
    auto asic =
        checkSameAndGetAsicForTesting(this->getAgentEnsemble()->getL3Asics());
    return pushedLabelStack(baseLabel, asic->getMaxLabelStackDepth());
  }

  void verifyCapturedLabelStack(
      const std::vector<MPLSHdr::Label>& labelStack,
      const LabelForwardingAction::LabelStack& expectedPushStack) const {
    ASSERT_EQ(labelStack.size(), expectedPushStack.size());

    auto actualLabels =
        utility::mpls_dataplane_test::capturedLabelValues(labelStack);
    auto expectedLabels =
        utility::mpls_dataplane_test::expectedWireOrderLabelValues(
            expectedPushStack);
    XLOG(INFO) << "MPLS dataplane PUSH captured labels "
               << utility::mpls_dataplane_test::labelValuesStr(actualLabels)
               << ", expected wire labels "
               << utility::mpls_dataplane_test::labelValuesStr(expectedLabels);

    EXPECT_EQ(actualLabels, expectedLabels);
    EXPECT_TRUE(
        utility::mpls_dataplane_test::bottomOfStackBitsValid(labelStack));
  }

  template <typename SendPacketFn>
  void verifyMplsPushAndTrapPacket(
      const std::string& snooperName,
      bool isV4,
      std::optional<PortID> injectPort,
      utility::mpls_dataplane_test::MplsTrapPacketMechanism mechanism,
      const LabelForwardingAction::LabelStack& expectedPushStack,
      SendPacketFn sendPacket) {
    SCOPED_TRACE(
        folly::to<std::string>(
            "ipVersion=",
            isV4 ? "IPv4" : "IPv6",
            " send=",
            injectPort.has_value() ? "front-panel" : "cpu",
            " trapMechanism=",
            utility::mpls_dataplane_test::name(mechanism),
            " isTrunk=",
            kIsTrunk));

    utility::SwSwitchPacketSnooper snooper(
        this->getSw(),
        snooperName,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        utility::packetSnooperReceivePacketType::PACKET_TYPE_ALL);
    snooper.ignoreUnclaimedRxPkts();

    auto cpuQueueOutPktsBefore = utility::getQueueOutPacketsWithRetry(
        this->getSw(),
        this->switchIdForPort(egressPort()),
        utility::kCoppLowPriQueueId,
        0 /* retryTimes */,
        0 /* expectedNumPkts */);
    auto outPktsBefore =
        utility::getPortOutPkts(this->getLatestPortStats(egressPort()));

    auto trapOnTtlExpiry = mechanism ==
        utility::mpls_dataplane_test::MplsTrapPacketMechanism::TtlExpiry;
    auto ttl = trapOnTtlExpiry ? 2 : 128;
    sendPacket(ttl);

    WITH_RETRIES({
      auto outPktsAfter =
          utility::getPortOutPkts(this->getLatestPortStats(egressPort()));
      EXPECT_EVENTUALLY_EQ(1, outPktsAfter - outPktsBefore);

      if (trapOnTtlExpiry) {
        auto cpuQueueOutPktsAfter = utility::getQueueOutPacketsWithRetry(
            this->getSw(),
            this->switchIdForPort(egressPort()),
            utility::kCoppLowPriQueueId,
            kGetQueueOutPktsRetryTimes,
            cpuQueueOutPktsBefore + 1);
        EXPECT_EVENTUALLY_EQ(1, cpuQueueOutPktsAfter - cpuQueueOutPktsBefore);
      }
    });

    auto pktBuf = snooper.waitForPacket(10);
    ASSERT_TRUE(pktBuf.has_value());
    ASSERT_TRUE(*pktBuf);
    XLOG(INFO) << "MPLS dataplane PUSH trapped packet hexdump:\n"
               << folly::hexDump((*pktBuf)->data(), (*pktBuf)->length());

    folly::io::Cursor cursor((*pktBuf).get());
    utility::EthFrame frame(cursor);

    auto mplsPayload = frame.mplsPayLoad();
    ASSERT_TRUE(mplsPayload.has_value());

    const auto& mplsHeader = mplsPayload->header();
    const auto& labelStack = mplsHeader.stack();
    XLOG(INFO) << "MPLS dataplane PUSH captured header " << mplsHeader;

    verifyCapturedLabelStack(labelStack, expectedPushStack);
  }
};

} // namespace facebook::fboss
