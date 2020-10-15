// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmLabeledEgress.h"

#include "fboss/agent/hw/bcm/BcmEgressManager.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmLabeledTunnel.h"
#include "fboss/agent/hw/bcm/BcmLabeledTunnelEgress.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <algorithm>

extern "C" {
#include <bcm/l3.h>
#include <bcm/mpls.h>
}

using namespace facebook::fboss;

namespace {
bcm_mpls_label_t getLabel(const bcm_l3_egress_t* egress) {
  return reinterpret_cast<const bcm_l3_egress_t*>(egress)->mpls_label;
}
std::array<bcm_mpls_label_t, 4> kLabels{
    201,
    301,
    401,
    501,
};

std::map<bcm_mpls_label_t, int> kLabelIndex{
    {201, 0},
    {301, 1},
    {401, 2},
    {501, 3},
};
std::vector<LabelForwardingAction::LabelStack> kLabelStacks{
    {202, 203},
    {302, 303},
    {402, 403},
    {502, 503},
};

std::array<bcm_if_t, 4> kIntfs{
    utility::kBaseVlanId,
    utility::kBaseVlanId,
    utility::kBaseVlanId + 1,
    utility::kBaseVlanId + 1,
};

std::array<folly::MacAddress, 4> kNextHopMacs{
    folly::MacAddress("1:2:3:4:5:6"),
    folly::MacAddress("1:2:3:4:5:7"),
    folly::MacAddress("1:2:3:4:5:8"),
    folly::MacAddress("1:2:3:4:5:9"),
};

std::array<folly::IPAddressV4, 4> kNextHopIps{
    folly::IPAddressV4("10.0.2.201"),
    folly::IPAddressV4("10.0.2.202"),
    folly::IPAddressV4("10.0.2.203"),
    folly::IPAddressV4("10.0.2.204"),
};
} // namespace

class BcmLabeledEgressTest : public BcmTest {
 public:
  void TearDown() override {
    ecmpEgresses_.clear();
    labeledEgresses_.clear();
    unlabeledEgresses_.clear();
    BcmTest::TearDown();
  }

  static int bcm_l3_egress_traverse_cb(
      int /*unit*/,
      bcm_if_t /*intf*/,
      bcm_l3_egress_t* info,
      void* user_data) {
    if (!user_data) {
      return 0;
    }
    auto* outvec = static_cast<std::vector<bcm_l3_egress_t>*>(user_data);
    outvec->push_back(*info);
    return 0;
  }

  static int bcm_l3_egress_ecmp_traverse_cb(
      int /*unit*/,
      bcm_l3_egress_ecmp_t* /*ecmp*/,
      int intf_count,
      bcm_if_t* intf_array,
      void* user_data) {
    if (!user_data) {
      return 0;
    }
    auto* outvec = reinterpret_cast<std::vector<bcm_if_t>*>(user_data);
    for (auto i = 0; i < intf_count; i++) {
      outvec->push_back(intf_array[i]);
    }
    return 0;
  }

  BcmLabeledEgress* makeLabeledEgress(bcm_mpls_label_t label) {
    auto var = std::make_unique<BcmLabeledEgress>(getHwSwitch(), label);
    auto* rv = var.get();
    labeledEgresses_.push_back(std::move(var));
    return rv;
  }

  BcmLabeledTunnelEgress* makeLabeledTunnelEgress(
      bcm_mpls_label_t label,
      LabelForwardingAction::LabelStack stack,
      bcm_if_t intf) {
    auto var = std::make_unique<BcmLabeledTunnelEgress>(
        getHwSwitch(), label, intf, std::move(stack));
    auto* rv = var.get();
    labeledEgresses_.push_back(std::move(var));
    return rv;
  }

  BcmEgress* makeUnlabeledEgress() {
    auto var = std::make_unique<BcmEgress>(getHwSwitch());
    auto* rv = var.get();
    unlabeledEgresses_.push_back(std::move(var));
    return rv;
  }

  BcmEcmpEgress* makeEcmpEgress(
      const BcmEcmpEgress::EgressId2Weight& egressId2Weight) {
    auto ecmpEgress =
        std::make_unique<BcmEcmpEgress>(getHwSwitch(), egressId2Weight);
    auto* rv = ecmpEgress.get();
    ecmpEgresses_.push_back(std::move(ecmpEgress));
    return rv;
  }

  void
  programEgressToPort(BcmEgress* egress, int index, bool setResolved = true) {
    egress->programToPort(
        getHwSwitch()
            ->getIntfTable()
            ->getBcmIntf(InterfaceID(kIntfs[index]))
            ->getBcmIfId(),
        0,
        kNextHopIps[index],
        kNextHopMacs[index],
        getHwSwitch()->getPortTable()->getBcmPortId(
            PortID(masterLogicalPortIds()[index])));

    if (setResolved) {
      getHwSwitch()->writableEgressManager()->resolved(egress->getID());
    }
  }

  void verifyTunnel(
      const bcm_l3_egress_t& egress,
      LabelForwardingAction::LabelStack stack) {
    std::array<bcm_mpls_egress_label_t, 10> label_array;
    int label_count = 0;
    bcm_mpls_tunnel_initiator_get(
        getHwSwitch()->getUnit(),
        egress.intf,
        10,
        label_array.data(),
        &label_count);
    EXPECT_EQ(label_count, stack.size() - 1);
    for (auto i = 0; i < label_count; i++) {
      EXPECT_EQ(label_array[i].label, stack[i + 1]);
    }
  }

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::twoL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }
  std::vector<std::unique_ptr<BcmEgress>> unlabeledEgresses_;
  std::vector<std::unique_ptr<BcmLabeledEgress>> labeledEgresses_;
  std::vector<std::unique_ptr<BcmEcmpEgress>> ecmpEgresses_;
};

TEST_F(BcmLabeledEgressTest, labeledEgress) {
  auto setup = [=]() {
    auto state = applyNewConfig(initialConfig());
    auto egress = makeLabeledEgress(kLabels[0]);
    programEgressToPort(egress, 0 /* index */);
    return state;
  };
  auto verify = [=]() {
    std::vector<bcm_l3_egress_t> egresses;
    bcm_l3_egress_traverse(
        getHwSwitch()->getUnit(),
        &BcmLabeledEgressTest::bcm_l3_egress_traverse_cb,
        &egresses);
    bool found = false;
    for (const auto& egress : egresses) {
      if (getLabel(&egress) == kLabels[0]) {
        found = true;
        break;
      }
    }
    EXPECT_EQ(found, true);
  };
  setup();
  verify();
}

TEST_F(BcmLabeledEgressTest, noLabeledEgress) {
  auto setup = [=]() {
    auto state = applyNewConfig(initialConfig());
    auto egress = makeUnlabeledEgress();
    programEgressToPort(egress, 0 /* index */);
    return state;
  };
  auto verify = [=]() {
    std::vector<bcm_l3_egress_t> out;
    bcm_l3_egress_traverse(
        getHwSwitch()->getUnit(),
        &BcmLabeledEgressTest::bcm_l3_egress_traverse_cb,
        &out);
    bool found = false;
    for (const auto& egress : out) {
      EXPECT_EQ(
          getLabel(&egress), static_cast<uint32_t>(BCM_MPLS_LABEL_INVALID));
      if (getLabel(&egress) != static_cast<uint32_t>(BCM_MPLS_LABEL_INVALID)) {
        found = true;
        break;
      }
    }
    EXPECT_EQ(found, false);
  };
  setup();
  verify();
}

TEST_F(BcmLabeledEgressTest, LabeledEgressWithEcmp) {
  auto setup = [=]() {
    auto state = applyNewConfig(initialConfig());
    BcmEcmpEgress::EgressId2Weight egressId2Weight;
    for (auto i = 0; i < 4; i++) {
      auto* egress = makeLabeledEgress(kLabels[i]);
      programEgressToPort(egress, i);
      egressId2Weight[egress->getID()] = 1;
    }
    makeEcmpEgress(egressId2Weight);
    return state;
  };
  auto verify = [=]() {
    std::vector<bcm_if_t> ecmpMembers;
    bcm_l3_egress_ecmp_traverse(
        getHwSwitch()->getUnit(),
        &BcmLabeledEgressTest::bcm_l3_egress_ecmp_traverse_cb,
        &ecmpMembers);

    EXPECT_EQ(ecmpMembers.size(), 4);

    for (const auto ecmpMember : ecmpMembers) {
      bcm_l3_egress_t egress;
      bcm_l3_egress_get(getHwSwitch()->getUnit(), ecmpMember, &egress);
      EXPECT_NE(
          std::end(kLabels),
          std::find(
              std::begin(kLabels),
              std::end(kLabels),
              getLabel(&egress))); // must find label
    }
  };
  setup();
  verify();
}

TEST_F(BcmLabeledEgressTest, labeledTunnelEgressWithEcmp) {
  auto setup = [=]() {
    auto state = applyNewConfig(initialConfig());
    std::vector<BcmEgress*> egresses;
    for (auto i = 0; i < 4; i++) {
      auto* intf =
          getHwSwitch()->getIntfTable()->getBcmIntfIf(InterfaceID(kIntfs[i]));
      egresses.push_back(makeLabeledTunnelEgress(
          kLabels[i], kLabelStacks[i], intf->getBcmIfId()));
    }
    BcmEcmpEgress::EgressId2Weight egressId2Weight;
    for (auto i = 0; i < 4; i++) {
      programEgressToPort(egresses[i], i);
      egressId2Weight[egresses[i]->getID()] = 1;
    }
    makeEcmpEgress(egressId2Weight);
    return state;
  };
  auto verify = [=]() {
    std::vector<bcm_if_t> ecmpMembers;
    bcm_l3_egress_ecmp_traverse(
        getHwSwitch()->getUnit(),
        &BcmLabeledEgressTest::bcm_l3_egress_ecmp_traverse_cb,
        &ecmpMembers);

    EXPECT_EQ(ecmpMembers.size(), 4);

    for (const auto ecmpMember : ecmpMembers) {
      bcm_l3_egress_t egress;
      bcm_l3_egress_get(getHwSwitch()->getUnit(), ecmpMember, &egress);
      auto label = getLabel(&egress);
      verifyTunnel(egress, kLabelStacks[kLabelIndex[label]]);
    }
    for (auto i = 0; i < 4; i++) {
      auto* intf =
          getHwSwitch()->getIntfTable()->getBcmIntf(InterfaceID(kIntfs[i]));
      EXPECT_EQ(
          intf->getLabeledTunnelCount(),
          2); // note that kIntf[0] & kIntf[1] are same and so are kIntf[2] and
              // kIntf[3]
    }
  };
  setup();
  verify();
}

TEST_F(BcmLabeledEgressTest, labeledTunnelEgressCommonTunnel) {
  auto setup = [=]() {
    auto state = applyNewConfig(initialConfig());
    std::vector<BcmEgress*> egresses;
    for (auto i = 0; i < 4; i++) {
      auto* intf =
          getHwSwitch()->getIntfTable()->getBcmIntfIf(InterfaceID(kIntfs[0]));
      egresses.push_back(makeLabeledTunnelEgress(
          kLabelStacks[0][0], kLabelStacks[0], intf->getBcmIfId()));
    }
    BcmEcmpEgress::EgressId2Weight egressId2Weight;
    for (auto i = 0; i < 4; i++) {
      programEgressToPort(egresses[i], i);
      egressId2Weight[egresses[i]->getID()] = 1;
    }
    makeEcmpEgress(egressId2Weight);
    return state;
  };
  auto verify = [=]() {
    std::vector<bcm_if_t> ecmpMembers;
    bcm_l3_egress_ecmp_traverse(
        getHwSwitch()->getUnit(),
        &BcmLabeledEgressTest::bcm_l3_egress_ecmp_traverse_cb,
        &ecmpMembers);

    EXPECT_EQ(ecmpMembers.size(), 4);

    for (const auto ecmpMember : ecmpMembers) {
      bcm_l3_egress_t egress;
      bcm_l3_egress_get(getHwSwitch()->getUnit(), ecmpMember, &egress);
      verifyTunnel(egress, kLabelStacks[0]);
    }
    auto* intf =
        getHwSwitch()->getIntfTable()->getBcmIntf(InterfaceID(kIntfs[0]));
    EXPECT_EQ(
        intf->getLabeledTunnelCount(),
        1); // only kLabelStacks[0] over kIntfs[0]
  };
  setup();
  verify();
}

TEST_F(BcmLabeledEgressTest, labeledTunnelEgressWithOneLabel) {
  LabelForwardingAction::LabelStack stack{101};
  auto setup = [=]() {
    auto state = applyNewConfig(initialConfig());
    std::vector<BcmEgress*> egresses;
    for (auto i = 0; i < 4; i++) {
      auto* intf =
          getHwSwitch()->getIntfTable()->getBcmIntfIf(InterfaceID(kIntfs[0]));
      egresses.push_back(
          makeLabeledTunnelEgress(stack[0], stack, intf->getBcmIfId()));
    }
    BcmEcmpEgress::EgressId2Weight egressId2Weight;
    for (auto i = 0; i < 4; i++) {
      programEgressToPort(egresses[i], i);
      egressId2Weight[egresses[i]->getID()] = 1;
    }
    makeEcmpEgress(egressId2Weight);
    return state;
  };
  auto verify = [=]() {
    std::vector<bcm_if_t> ecmpMembers;
    bcm_l3_egress_ecmp_traverse(
        getHwSwitch()->getUnit(),
        &BcmLabeledEgressTest::bcm_l3_egress_ecmp_traverse_cb,
        &ecmpMembers);

    EXPECT_EQ(ecmpMembers.size(), 4);

    for (const auto ecmpMember : ecmpMembers) {
      bcm_l3_egress_t egress;
      bcm_l3_egress_get(getHwSwitch()->getUnit(), ecmpMember, &egress);
      verifyTunnel(egress, stack);
    }
    auto* intf =
        getHwSwitch()->getIntfTable()->getBcmIntf(InterfaceID(kIntfs[0]));
    EXPECT_EQ(
        intf->getLabeledTunnelCount(),
        1); // only kLabelStacks[0] over kIntfs[0]
  };
  setup();
  verify();
}

TEST_F(BcmLabeledEgressTest, LabeledUnlabeledEgressWithEcmp) {
  auto setup = [=]() {
    auto state = applyNewConfig(initialConfig());
    std::vector<BcmEgress*> egresses;
    egresses.push_back(makeLabeledEgress(kLabels[0]));
    egresses.push_back(makeLabeledEgress(kLabels[1]));
    egresses.push_back(makeUnlabeledEgress());
    egresses.push_back(makeUnlabeledEgress());
    BcmEcmpEgress::EgressId2Weight egressId2Weight;
    for (auto i = 0; i < 4; i++) {
      programEgressToPort(egresses[i], i);
      egressId2Weight[egresses[i]->getID()] = 1;
    }
    makeEcmpEgress(egressId2Weight);
    return state;
  };
  auto verify = [=]() {
    std::vector<bcm_if_t> ecmpMembers;
    bcm_l3_egress_ecmp_traverse(
        getHwSwitch()->getUnit(),
        &BcmLabeledEgressTest::bcm_l3_egress_ecmp_traverse_cb,
        &ecmpMembers);

    EXPECT_EQ(ecmpMembers.size(), 4);

    auto labeled = 0;
    auto unlabeled = 0;
    for (const auto ecmpMember : ecmpMembers) {
      bcm_l3_egress_t egress;
      bcm_l3_egress_get(getHwSwitch()->getUnit(), ecmpMember, &egress);
      auto label = getLabel(&egress);
      if (label == static_cast<uint32_t>(BCM_MPLS_LABEL_INVALID)) {
        unlabeled++;
      } else if (label == kLabels[0] || label == kLabels[1]) {
        labeled++;
      }
    }
    EXPECT_EQ((labeled + unlabeled), 4);
  };
  setup();
  verify();
}
