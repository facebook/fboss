// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/QosTestUtils.h"

#include <folly/IPAddressV4.h>
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <gtest/gtest.h>
#include <chrono>

namespace facebook::fboss::utility {

namespace {
template <typename AddrT>
void setDisableTTLDecrementOnNeighnor(
    std::shared_ptr<SwitchState>* state,
    InterfaceID intfID,
    AddrT ip) {
  auto intf = (*state)->getInterfaces()->getNodeIf(intfID);
  auto vlan = (*state)->getVlans()->getNodeIf(intf->getVlanID())->modify(state);

  if (vlan->template getNeighborEntryTable<AddrT>()->getEntryIf(ip)) {
    auto nbrTable =
        vlan->template getNeighborEntryTable<AddrT>()->modify(&vlan, state);
    auto entry = nbrTable->getEntryIf(ip)->clone();
    entry->setDisableTTLDecrement(true);
    nbrTable->updateNode(entry);
  } else if (intf->template getNeighborEntryTable<AddrT>()->getEntryIf(ip)) {
    auto nbrTable =
        intf->template getNeighborEntryTable<AddrT>()->modify(intfID, state);
    auto entry = nbrTable->getEntryIf(ip)->clone();
    entry->setDisableTTLDecrement(true);
    nbrTable->updateNode(entry);
  }
}
} // namespace

std::shared_ptr<SwitchState> disableTTLDecrement(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    RouterID routerId,
    InterfaceID intfID,
    const folly::IPAddress& nhop) {
  // TODO: get the scope of the interface from the state and verify it is
  // supported on ASIC of that switch
  if (!asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE)) {
    throw FbossError("Next hop ttl decrement is not supported on this ASIC");
  }
  auto newState = state->clone();
  for (auto iter : std::as_const(*newState->getFibs())) {
    auto fibMap = iter.second;
    auto fibContainer = fibMap->getFibContainerIf(routerId);
    if (!fibContainer) {
      continue;
    }
    auto v4Fib = fibContainer->getFibV4()->modify(routerId, &newState);
    auto v6Fib = fibContainer->getFibV6()->modify(routerId, &newState);
    v4Fib->disableTTLDecrement(nhop);
    v6Fib->disableTTLDecrement(nhop);
  }

  if (nhop.isV4()) {
    setDisableTTLDecrementOnNeighnor(&newState, intfID, nhop.asV4());
  } else {
    setDisableTTLDecrementOnNeighnor(&newState, intfID, nhop.asV6());
  }
  return newState;
}

std::shared_ptr<SwitchState> disableTTLDecrement(
    HwAsicTable* asicTable,
    const std::shared_ptr<SwitchState>& state,
    const PortDescriptor& port) {
  // TODO: get the scope of the port from the state and verify it is supported
  // on ASIC of that switch
  if (!asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE)) {
    throw FbossError("port ttl decrement is not supported on this ASIC");
  }
  if (!port.isPhysicalPort()) {
    throw FbossError("Cannot ttl decrement for non-physical port");
  }
  auto newState = state->clone();
  auto phyPort =
      state->getPorts()->getNodeIf(port.phyPortID())->modify(&newState);
  phyPort->setTTLDisableDecrement(true);
  return newState;
}

void disableTTLDecrements(TestEnsembleIf* hw, const PortDescriptor& port) {
  hw->applyNewState(
      [asicTable = hw->getHwAsicTable(),
       port](const std::shared_ptr<SwitchState>& state) {
        auto newState = disableTTLDecrement(asicTable, state, port);
        return newState;
      },
      "Disable TTL decrements on port");
}

void disableTTLDecrements(
    TestEnsembleIf* hw,
    RouterID routerId,
    InterfaceID intf,
    const folly::IPAddress& nhop) {
  hw->applyNewState(
      [=, asicTable = hw->getHwAsicTable()](
          const std::shared_ptr<SwitchState>& state) {
        auto newState =
            disableTTLDecrement(asicTable, state, routerId, intf, nhop);
        return newState;
      },
      "Disable TTL decrements on next hop: " + nhop.str());
}

void disableTTLDecrementOnPorts(
    SwSwitch* sw,
    const boost::container::flat_set<PortDescriptor>& ecmpPorts) {
  auto asicTable = sw->getHwAsicTable();
  sw->updateStateBlocking(
      "Disable TTL Decrement On Ports",
      [ecmpPorts, asicTable](const std::shared_ptr<SwitchState>& state) {
        auto newState = state->clone();
        for (auto port : ecmpPorts) {
          newState = disableTTLDecrement(asicTable, newState, port);
        }
        return newState;
      });
}

/*
 * Disable TTL decrement on a set of ports
 */
void disableTTLDecrements(
    SwSwitch* sw,
    const boost::container::flat_set<PortDescriptor>& ecmpPorts) {
  std::set<SwitchID> switchIds;
  for (auto port : ecmpPorts) {
    auto switchId = sw->getScopeResolver()->scope(port.phyPortID()).switchId();
    if (sw->getHwAsicTable()->isFeatureSupported(
            switchId, HwAsic::Feature::SAI_TTL0_PACKET_FORWARD_ENABLE)) {
    } else if (sw->getHwAsicTable()->isFeatureSupported(
                   switchId, HwAsic::Feature::PORT_TTL_DECREMENT_DISABLE)) {
      disableTTLDecrementOnPorts(sw, {port});
    } else if (sw->getHwAsicTable()->isFeatureSupported(
                   switchId, HwAsic::Feature::NEXTHOP_TTL_DECREMENT_DISABLE)) {
      utility::EcmpSetupTargetedPorts6 ecmp6(
          sw->getState(),
          sw->needL2EntryForNeighbor(),
          sw->getLocalMac(switchId),
          RouterID(0),
          false,
          {cfg::PortType::INTERFACE_PORT, cfg::PortType::MANAGEMENT_PORT});
      disableTTLDecrements(sw, ecmp6.getRouterId(), ecmp6.nhop(port));
    } else {
      throw FbossError("Failed to configure TTL decrement");
    }
  }
}

void verifyQueueHit(
    const HwPortStats& portStatsBefore,
    int queueId,
    SwSwitch* sw,
    facebook::fboss::PortID egressPort,
    int delta) {
  auto queuePacketsBefore = portStatsBefore.queueOutPackets_()->find(queueId) !=
          portStatsBefore.queueOutPackets_()->end()
      ? portStatsBefore.queueOutPackets_()->find(queueId)->second
      : 0;
  WITH_RETRIES({
    int64_t queuePacketsAfter = 0;
    auto latestPortStats = sw->getHwPortStats({egressPort});
    if (latestPortStats.find(egressPort) != latestPortStats.end()) {
      auto portStatsAfter = latestPortStats[egressPort];
      if (portStatsAfter.queueOutPackets_()->find(queueId) !=
          portStatsAfter.queueOutPackets_()->end()) {
        queuePacketsAfter = portStatsAfter.queueOutPackets_()[queueId];
      }
    }
    EXPECT_EVENTUALLY_GE(queuePacketsAfter, queuePacketsBefore + delta);
    // Note, on some platforms, due to how loopbacked packets are pruned
    // from being broadcast, they will appear more than once on a queue
    // counter, so we can only check that the counter went up, not that it
    // went up by exactly one.
    XLOG(DBG2) << "Port ID: " << egressPort << " queue: " << queueId
               << " queuePacketsBefore " << queuePacketsBefore
               << " queuePacketsAfter " << queuePacketsAfter << " delta "
               << delta;
  });
}

void verifyVoQHit(
    const HwSysPortStats& portStatsBefore,
    int queueId,
    SwSwitch* sw,
    facebook::fboss::SystemPortID egressPort,
    int delta) {
  auto l3Asics = sw->getHwAsicTable()->getL3Asics();
  auto asic = checkSameAndGetAsic(l3Asics);
  int voqId = getTrafficClassToVoqId(asic, queueId);
  auto queueBytesBefore = portStatsBefore.queueOutBytes_()->find(voqId) !=
          portStatsBefore.queueOutBytes_()->end()
      ? portStatsBefore.queueOutBytes_()->find(voqId)->second
      : 0;
  WITH_RETRIES({
    int64_t queueBytesAfter = 0;
    auto latestPortStats = sw->getHwSysPortStats({egressPort});
    if (latestPortStats.find(egressPort) != latestPortStats.end()) {
      auto portStatsAfter = latestPortStats[egressPort];
      for (int i = 0; i < 8; i++) {
        if (portStatsAfter.queueOutBytes_()->find(i) ==
            portStatsAfter.queueOutBytes_()->end()) {
          XLOG(DBG2) << "no voq id " << i;
          continue;
        }
        queueBytesAfter = portStatsAfter.queueOutBytes_()[i];
        XLOG(DBG2) << "Sys port: " << egressPort << " voq " << i
                   << " queueBytesBefore " << queueBytesBefore
                   << " queueBytesAfter " << queueBytesAfter << " delta "
                   << delta;
      }
      if (portStatsAfter.queueOutBytes_()->find(voqId) !=
          portStatsAfter.queueOutBytes_()->end()) {
        queueBytesAfter = portStatsAfter.queueOutBytes_()[voqId];
      }
    }
    EXPECT_EVENTUALLY_GE(queueBytesAfter, queueBytesBefore + delta);
  });
}

bool verifyQueueMappings(
    const HwPortStats& portStatsBefore,
    const std::map<int, std::vector<uint8_t>>& q2dscps,
    std::function<std::map<PortID, HwPortStats>()> getAllHwPortStats,
    const PortID portId,
    uint32_t sleep) {
  WITH_RETRIES_N_TIMED(10, std::chrono::milliseconds(sleep), {
    auto portStatsAfter = getAllHwPortStats();
    for (const auto& _q2dscps : q2dscps) {
      auto queuePacketsBefore =
          portStatsBefore.queueOutPackets_()->find(_q2dscps.first)->second;
      auto queuePacketsAfter =
          portStatsAfter[portId].queueOutPackets_()[_q2dscps.first];
      // Note, on some platforms, due to how loopbacked packets are pruned
      // from being broadcast, they will appear more than once on a queue
      // counter, so we can only check that the counter went up, not that it
      // went up by exactly one.
      XLOG(DBG2) << "queue " << _q2dscps.first << " queuePacketsBefore "
                 << queuePacketsBefore << " queuePacketsAfter "
                 << queuePacketsAfter;
      EXPECT_EVENTUALLY_TRUE(
          queuePacketsAfter >= queuePacketsBefore + _q2dscps.second.size());
      return true;
    }
  });
  return false;
}

int getMaxWeightWRRQueue(const std::map<int, uint8_t>& queueToWeight) {
  auto maxItr = std::max_element(
      queueToWeight.begin(),
      queueToWeight.end(),
      [](const std::pair<int, uint64_t>& p1,
         const std::pair<int, uint64_t>& p2) { return p1.second < p2.second; });

  return maxItr->first;
}

} // namespace facebook::fboss::utility
