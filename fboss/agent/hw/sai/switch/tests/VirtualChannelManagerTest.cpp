/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualChannelManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/state/Port.h"

#include <gtest/gtest.h>

#if defined(SAI_CBFC_SUPPORTED)

using namespace facebook::fboss;

namespace {

// The two lossless classes SUSWs run today: pg2 rdma, pg6 monitoring.
state::PortVcFields makeVc(
    int16_t id,
    std::optional<int64_t> reservedCreditSize) {
  state::PortVcFields vc;
  vc.id() = id;
  vc.senderEnable() = true;
  vc.receiverEnable() = true;
  if (reservedCreditSize) {
    vc.reservedCreditSize() = *reservedCreditSize;
  }
  return vc;
}

} // namespace

class VirtualChannelManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::BLANK;
    ManagerTestBase::SetUp();
    // ManagerTestBase declares fs but never assigns it.
    fs = FakeSai::getInstance();
    p0 = testInterfaces[0].remoteHosts[0].port;
    p1 = testInterfaces[1].remoteHosts[0].port;
  }

  std::shared_ptr<Port> makePortWithVcs(
      const TestPort& testPort,
      std::vector<state::PortVcFields> vcs) {
    auto swPort = makePort(testPort);
    swPort->setVirtualChannels(std::move(vcs));
    return swPort;
  }

  const FakeVirtualChannel& getVc(sai_uint8_t index) const {
    for (const auto& [_, vc] : fs->virtualChannelManager.map()) {
      if (vc.getIndex() == index) {
        return vc;
      }
    }
    throw FbossError("no fake virtual channel with index ", index);
  }

  size_t vcCount() const {
    return fs->virtualChannelManager.map().size();
  }

  size_t profileCount() const {
    return fs->cbfcCreditProfileManager.map().size();
  }

  TestPort p0;
  TestPort p1;
};

TEST_F(VirtualChannelManagerTest, addPortCreatesVirtualChannels) {
  auto swPort = makePortWithVcs(p0, {makeVc(2, 36), makeVc(6, 12)});
  saiManagerTable->portManager().addPort(swPort);

  EXPECT_EQ(vcCount(), 2);
  EXPECT_TRUE(getVc(2).getCbfcSenderEnable());
  EXPECT_TRUE(getVc(6).getCbfcReceiverEnable());
}

TEST_F(VirtualChannelManagerTest, virtualChannelsBindToTheirPort) {
  auto swPort = makePortWithVcs(p0, {makeVc(2, 36)});
  saiManagerTable->portManager().addPort(swPort);

  auto* handle = saiManagerTable->portManager().getPortHandle(swPort->getID());
  ASSERT_NE(handle, nullptr);
  EXPECT_EQ(getVc(2).getPort(), handle->port->adapterKey());
}

TEST_F(VirtualChannelManagerTest, portWithoutCbfcCreatesNothing) {
  auto swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  EXPECT_EQ(vcCount(), 0);
  EXPECT_EQ(profileCount(), 0);
}

TEST_F(VirtualChannelManagerTest, creditProfilesAreSharedAcrossPorts) {
  // Both ports ask for the same reservation, so they share one profile object.
  saiManagerTable->portManager().addPort(makePortWithVcs(p0, {makeVc(2, 36)}));
  saiManagerTable->portManager().addPort(makePortWithVcs(p1, {makeVc(2, 36)}));

  EXPECT_EQ(vcCount(), 2);
  EXPECT_EQ(profileCount(), 1);
}

TEST_F(VirtualChannelManagerTest, distinctReservationsGetDistinctProfiles) {
  auto swPort = makePortWithVcs(p0, {makeVc(2, 36), makeVc(6, 12)});
  saiManagerTable->portManager().addPort(swPort);

  EXPECT_EQ(profileCount(), 2);
  EXPECT_NE(
      getVc(2).getCbfcSenderCreditProfile(),
      getVc(6).getCbfcSenderCreditProfile());
}

TEST_F(VirtualChannelManagerTest, vcWithoutReservationHasNoProfile) {
  auto swPort = makePortWithVcs(p0, {makeVc(2, std::nullopt)});
  saiManagerTable->portManager().addPort(swPort);

  EXPECT_EQ(profileCount(), 0);
  EXPECT_EQ(getVc(2).getCbfcSenderCreditProfile(), SAI_NULL_OBJECT_ID);
}

TEST_F(VirtualChannelManagerTest, changePortAddsAVirtualChannel) {
  auto swPort = makePortWithVcs(p0, {makeVc(2, 36)});
  saiManagerTable->portManager().addPort(swPort);
  ASSERT_EQ(vcCount(), 1);

  auto changedPort = swPort->clone();
  changedPort->setVirtualChannels(
      std::vector<state::PortVcFields>{makeVc(2, 36), makeVc(6, 12)});
  saiManagerTable->portManager().changePort(swPort, changedPort);

  EXPECT_EQ(vcCount(), 2);
}

TEST_F(VirtualChannelManagerTest, changePortRemovesAVirtualChannel) {
  auto swPort = makePortWithVcs(p0, {makeVc(2, 36), makeVc(6, 12)});
  saiManagerTable->portManager().addPort(swPort);
  ASSERT_EQ(vcCount(), 2);

  // Rebuilding the handle with a smaller set is what drops the objects the
  // port no longer wants.
  auto changedPort = swPort->clone();
  changedPort->setVirtualChannels(
      std::vector<state::PortVcFields>{makeVc(2, 36)});
  saiManagerTable->portManager().changePort(swPort, changedPort);

  EXPECT_EQ(vcCount(), 1);
  EXPECT_EQ(getVc(2).getIndex(), 2);
}

TEST_F(VirtualChannelManagerTest, changedReservationRebindsProfile) {
  auto swPort = makePortWithVcs(p0, {makeVc(2, 36)});
  saiManagerTable->portManager().addPort(swPort);
  auto originalProfile = getVc(2).getCbfcSenderCreditProfile();

  // reservedCreditSize is CREATE_ONLY on the profile, so the manager has to
  // create a new profile and rebind rather than mutate the old one. The stale
  // profile must be gone once nothing references it.
  auto changedPort = swPort->clone();
  changedPort->setVirtualChannels(
      std::vector<state::PortVcFields>{makeVc(2, 48)});
  saiManagerTable->portManager().changePort(swPort, changedPort);

  EXPECT_NE(getVc(2).getCbfcSenderCreditProfile(), originalProfile);
  EXPECT_EQ(profileCount(), 1);
}

TEST_F(VirtualChannelManagerTest, changePortFlipsAnEnable) {
  auto swPort = makePortWithVcs(p0, {makeVc(2, 36)});
  saiManagerTable->portManager().addPort(swPort);
  auto originalId = getVc(2).id;

  auto vc = makeVc(2, 36);
  vc.senderEnable() = false;
  auto changedPort = swPort->clone();
  changedPort->setVirtualChannels(std::vector<state::PortVcFields>{vc});
  saiManagerTable->portManager().changePort(swPort, changedPort);

  // Same object, updated in place: the enables are CREATE_AND_SET.
  EXPECT_EQ(vcCount(), 1);
  EXPECT_EQ(getVc(2).id, originalId);
  EXPECT_FALSE(getVc(2).getCbfcSenderEnable());
}

TEST_F(VirtualChannelManagerTest, virtualChannelHandleExposesCreatedObjects) {
  auto swPort = makePortWithVcs(p0, {makeVc(2, 36), makeVc(6, 12)});
  saiManagerTable->portManager().addPort(swPort);

  auto* handle =
      saiManagerTable->virtualChannelManager().getVirtualChannelHandle(
          swPort->getID());
  ASSERT_NE(handle, nullptr);
  EXPECT_EQ(handle->virtualChannels.size(), 2);
  EXPECT_EQ(handle->creditProfiles.size(), 2);
}

TEST_F(VirtualChannelManagerTest, virtualChannelHandleNullForPortWithoutCbfc) {
  auto swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  EXPECT_EQ(
      saiManagerTable->virtualChannelManager().getVirtualChannelHandle(
          swPort->getID()),
      nullptr);
}

TEST_F(VirtualChannelManagerTest, removePortRemovesVirtualChannels) {
  auto swPort = makePortWithVcs(p0, {makeVc(2, 36), makeVc(6, 12)});
  saiManagerTable->portManager().addPort(swPort);
  ASSERT_EQ(vcCount(), 2);
  ASSERT_EQ(profileCount(), 2);

  saiManagerTable->portManager().removePort(swPort);

  EXPECT_EQ(vcCount(), 0);
  EXPECT_EQ(profileCount(), 0);
}

#endif
