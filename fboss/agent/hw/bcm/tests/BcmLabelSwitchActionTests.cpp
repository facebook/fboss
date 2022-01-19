// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabelSwitchAction.h"

#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <gtest/gtest.h>
#include "fboss/agent/hw/bcm/BcmLabelSwitchingUtils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwSwitchEnsembleRouteUpdateWrapper.h"
#include "fboss/agent/test/LabelForwardingUtils.h"

using namespace ::testing;

using namespace facebook::fboss;

class BcmLabelSwitchActionTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::twoL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }

  std::shared_ptr<SwitchState> initState() {
    auto state = applyNewConfig(initialConfig());
    setupTestLabelForwardingEntries();
    return state;
  }

  void setupTestLabelForwardingEntries() {
    entries_.push_back(std::make_shared<LabelForwardingEntry>(
        5001,
        ClientID::OPENR,
        util::getSwapLabelNextHopEntry(
            AdminDistance::DIRECTLY_CONNECTED,
            InterfaceID(utility::kBaseVlanId))));
    entries_.push_back(std::make_shared<LabelForwardingEntry>(
        5002,
        ClientID::OPENR,
        util::getPhpLabelNextHopEntry(
            AdminDistance::DIRECTLY_CONNECTED,
            InterfaceID(utility::kBaseVlanId))));
    entries_.push_back(std::make_shared<LabelForwardingEntry>(
        5003,
        ClientID::OPENR,
        util::getPopLabelNextHopEntry(
            AdminDistance::DIRECTLY_CONNECTED,
            InterfaceID(utility::kBaseVlanId))));
    entries_.push_back(std::make_shared<LabelForwardingEntry>(
        5004,
        ClientID::OPENR,
        util::getPushLabelNextHopEntry(
            AdminDistance::DIRECTLY_CONNECTED,
            InterfaceID(utility::kBaseVlanId))));
  }

  void addLabelForwardingEntry(
      std::shared_ptr<SwitchState>* state,
      std::shared_ptr<LabelForwardingEntry> entry) {
    auto* lFib = (*state)->getLabelForwardingInformationBase()->modify(state);
    lFib->addNode(std::move(entry));
  }

  void removeLabelForwardingEntry(
      std::shared_ptr<SwitchState>* state,
      const std::shared_ptr<LabelForwardingEntry>& entry) {
    auto* lFib = (*state)->getLabelForwardingInformationBase()->modify(state);
    auto deletingEntry = lFib->getLabelForwardingEntry(entry->getID());
    lFib->removeNode(deletingEntry);
  }

  void addAllTestLabelForwardingEntries() {
    auto oldState = initState();
    auto newState = oldState->clone();
    for (auto& entry : entries_) {
      LabelForwardingInformationBase::resolve(entry);
      addLabelForwardingEntry(&newState, entry);
    }
    applyNewState(newState);
  }

  void removeAllTestLabelForwardingEntries() {
    auto newState = getProgrammedState()->clone();
    for (auto& entry : entries_) {
      removeLabelForwardingEntry(&newState, entry);
    }
    applyNewState(newState);
  }

  void verifyTestLabelForwardingEntry(
      const std::shared_ptr<LabelForwardingEntry>& entry) {
    bcm_mpls_tunnel_switch_t info;
    bcm_mpls_tunnel_switch_t_init(&info);
    info.label = entry->getID().value();
    info.port = BCM_GPORT_INVALID;
    auto rv = bcm_mpls_tunnel_switch_get(getHwSwitch()->getUnit(), &info);
    ASSERT_EQ(rv, BCM_E_NONE);
    EXPECT_EQ(info.label, static_cast<int32_t>(entry->getID().value()));
    EXPECT_EQ(
        info.action,
        utility::getLabelSwitchAction(
            entry->getForwardInfo().getAction(),
            entry->getForwardInfo()
                .getNextHopSet()
                .begin()
                ->labelForwardingAction()
                ->type()));

    if (info.action == BCM_MPLS_SWITCH_ACTION_POP) {
      // no egress_if or next hop for "POP"
      return;
    }
    if (info.action != BCM_MPLS_SWITCH_ACTION_PHP) {
      // for push & swap, use labeled egresses
      auto reference = getHwSwitch()->getMultiPathNextHopTable()->getNextHop(
          BcmMultiPathNextHopKey(
              0, entry->getForwardInfo().normalizedNextHops()));
      EXPECT_EQ(info.egress_if, reference->getEgressId());
    } else {
      // for php reuse L3 egresses
      auto reference = getHwSwitch()->getMultiPathNextHopTable()->getNextHop(
          BcmMultiPathNextHopKey(
              0,
              utility::stripLabelForwarding(
                  entry->getForwardInfo().normalizedNextHops())));
      EXPECT_EQ(info.egress_if, reference->getEgressId());
    }
  }

  void verifyTestLabelForwardingEntryNotFound(
      const std::shared_ptr<LabelForwardingEntry>& entry) {
    bcm_mpls_tunnel_switch_t info;
    bcm_mpls_tunnel_switch_t_init(&info);
    info.label = entry->getID().value();
    info.port = BCM_GPORT_INVALID;
    auto rv = bcm_mpls_tunnel_switch_get(getHwSwitch()->getUnit(), &info);
    ASSERT_EQ(rv, BCM_E_NOT_FOUND);
  }

  void verifyAllTestLabelForwardingEntries() {
    for (const auto& entry : entries_) {
      verifyTestLabelForwardingEntry(entry);
    }
  }

  void verifyAllTestLabelForwardingEntriesNotFound() {
    for (const auto& entry : entries_) {
      verifyTestLabelForwardingEntryNotFound(entry);
    }
  }

  void addL3Route(
      const folly::IPAddressV4& network,
      uint8_t mask,
      const LabelForwardingEntry& entry) {
    RouteNextHopSet nexthops;
    for (const auto& nhop : entry.getForwardInfo().getNextHopSet()) {
      nexthops.insert(UnresolvedNextHop(nhop.addr(), nhop.weight()));
    }
    auto updater = getHwSwitchEnsemble()->getRouteUpdater();

    updater.addRoute(
        RouterID(0),
        network,
        mask,
        ClientID(786),
        RouteNextHopEntry(std::move(nexthops), AdminDistance::STATIC_ROUTE));
    updater.program();
  }

  void addL3Routes() {
    std::vector<folly::IPAddressV4> addrs{
        folly::IPAddressV4("10.1.101.10"),
        folly::IPAddressV4("10.1.102.10"),
        folly::IPAddressV4("10.1.103.10"),
        folly::IPAddressV4("10.1.104.10"),
    };
    for (auto i = 0; i < 4; i++) {
      addL3Route(addrs[i], 24, *entries_[i]);
    }
  }

  std::vector<std::shared_ptr<LabelForwardingEntry>> entries_;
};

TEST_F(BcmLabelSwitchActionTest, addLabelSwitchAction) {
  if (!isSupported(HwAsic::Feature::MPLS)) {
    return;
  }
  auto setup = [=]() { addAllTestLabelForwardingEntries(); };
  auto verify = [=]() { verifyAllTestLabelForwardingEntries(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmLabelSwitchActionTest, addLabelSwitchActionWithL3Routes) {
  if (!isSupported(HwAsic::Feature::MPLS) ||
      !isSupported(HwAsic::Feature::MPLS_ECMP)) {
    return;
  }
  auto setup = [=]() {
    addAllTestLabelForwardingEntries();
    addL3Routes();
  };
  auto verify = [=]() { verifyAllTestLabelForwardingEntries(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmLabelSwitchActionTest, removeLabelSwitchAction) {
  if (!isSupported(HwAsic::Feature::MPLS)) {
    return;
  }
  auto setup = [=]() {
    addAllTestLabelForwardingEntries();
    removeAllTestLabelForwardingEntries();
  };
  auto verify = [=]() { verifyAllTestLabelForwardingEntriesNotFound(); };

  verifyAcrossWarmBoots(setup, verify);
}
