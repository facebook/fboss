/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiMacsecManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/types.h"
#include "fboss/mka_service/if/gen-cpp2/mka_types.h"
#include "fboss/qsfp_service/util/MacsecUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

namespace {
static std::array<uint8_t, 12> kTestSalt{
    0x01,
    0x02,
    0x03,
    0x04,
    0x05,
    0x06,
    0x07,
    0x08,
    0x09,
    0x10,
    0x11,
    0x12};

const MacsecShortSecureChannelId kTestSsci =
    MacsecShortSecureChannelId(0x01010101);

} // namespace

namespace facebook::fboss {
class MacsecManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::BLANK;
    ManagerTestBase::SetUp();
    p0 = testInterfaces[0].remoteHosts[0].port;
    p1 = testInterfaces[1].remoteHosts[0].port;

    localSci = utility::makeSci("00:00:00:00:00:00", PortID(p0.id));
    remoteSci = utility::makeSci("11:11:11:11:11:11", PortID(p1.id));
    rxSak = utility::makeSak(
        remoteSci,
        // Just use the port number as the portName for now (macsecManager does
        // not rely on the interface name)
        folly::to<std::string>(p0.id),
        "01020304050607080910111213141516",
        "0807060504030201",
        0);
    txSak = utility::makeSak(
        localSci,
        folly::to<std::string>(p0.id),
        "16151413121110090807060504030201",
        "0102030405060708",
        0);

    std::copy(
        rxSak.keyHex_ref()->begin(),
        rxSak.keyHex_ref()->end(),
        rxSecureAssocKey.data());
    std::copy(
        rxSak.keyIdHex_ref()->begin(),
        rxSak.keyIdHex_ref()->end(),
        rxSecureAssocAuthKey.data());
  }

  TestPort p0;
  TestPort p1;

  mka::MKASak rxSak;
  mka::MKASak txSak;
  mka::MKASci localSci;
  mka::MKASci remoteSci;
  std::array<uint8_t, 32> rxSecureAssocKey;
  std::array<uint8_t, 16> rxSecureAssocAuthKey;
};

TEST_F(MacsecManagerTest, addMacsec) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_EGRESS, false);

  auto ingressHandle = saiManagerTable->macsecManager().getMacsecHandle(
      SAI_MACSEC_DIRECTION_INGRESS);
  auto egressHandle = saiManagerTable->macsecManager().getMacsecHandle(
      SAI_MACSEC_DIRECTION_EGRESS);

  CHECK_NE(ingressHandle, nullptr);
  CHECK_NE(egressHandle, nullptr);
}

TEST_F(MacsecManagerTest, addDuplicateMacsec) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);
  EXPECT_THROW(
      saiManagerTable->macsecManager().addMacsec(
          SAI_MACSEC_DIRECTION_INGRESS, false),
      FbossError);
}

TEST_F(MacsecManagerTest, removeMacsec) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  auto ingressHandle = saiManagerTable->macsecManager().getMacsecHandle(
      SAI_MACSEC_DIRECTION_INGRESS);
  CHECK_NE(ingressHandle, nullptr);

  saiManagerTable->macsecManager().removeMacsec(SAI_MACSEC_DIRECTION_INGRESS);

  ingressHandle = saiManagerTable->macsecManager().getMacsecHandle(
      SAI_MACSEC_DIRECTION_INGRESS);
  CHECK_EQ(ingressHandle, nullptr);
}

TEST_F(MacsecManagerTest, removeNonexistentMacsec) {
  EXPECT_THROW(
      saiManagerTable->macsecManager().removeMacsec(
          SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);
}

TEST_F(MacsecManagerTest, addMacsecPort) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_EGRESS, false);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);
  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_EGRESS);

  auto ingressPort = saiManagerTable->macsecManager().getMacsecPortHandle(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  auto egressPort = saiManagerTable->macsecManager().getMacsecPortHandle(
      swPort->getID(), SAI_MACSEC_DIRECTION_EGRESS);

  CHECK_NE(ingressPort, nullptr);
  CHECK_NE(egressPort, nullptr);
}

TEST_F(MacsecManagerTest, addMacsecPortForNonexistentMacsec) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  EXPECT_THROW(
      saiManagerTable->macsecManager().addMacsecPort(
          swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);
}

TEST_F(MacsecManagerTest, addMacsecPortForNonexistentPort) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  EXPECT_THROW(
      saiManagerTable->macsecManager().addMacsecPort(
          PortID(p0.id), SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);
}

TEST_F(MacsecManagerTest, addDuplicateMacsecPort) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);
  EXPECT_THROW(
      saiManagerTable->macsecManager().addMacsecPort(
          swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);
}

TEST_F(MacsecManagerTest, removeMacsecPort) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  auto ingressPort = saiManagerTable->macsecManager().getMacsecPortHandle(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  CHECK_NE(ingressPort, nullptr);

  saiManagerTable->macsecManager().removeMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  ingressPort = saiManagerTable->macsecManager().getMacsecPortHandle(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);
  CHECK_EQ(ingressPort, nullptr);
}

TEST_F(MacsecManagerTest, removeNonexistentMacsecPort) {
  // When there's no macsec pipeline obj
  EXPECT_THROW(
      saiManagerTable->macsecManager().removeMacsecPort(
          PortID(p0.id), SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);

  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  // When there's a macsec pipeline, but no macsecPort
  EXPECT_THROW(
      saiManagerTable->macsecManager().removeMacsecPort(
          PortID(p0.id), SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);
}

TEST_F(MacsecManagerTest, addMacsecSecureChannel) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  saiManagerTable->macsecManager().addMacsecSecureChannel(
      swPort->getID(),
      SAI_MACSEC_DIRECTION_INGRESS,
      utility::packSecureChannelId(remoteSci),
      true);

  auto scHandle = saiManagerTable->macsecManager().getMacsecSecureChannelHandle(
      swPort->getID(),
      utility::packSecureChannelId(remoteSci),
      SAI_MACSEC_DIRECTION_INGRESS);

  CHECK_NE(scHandle, nullptr);
  CHECK_NE(scHandle->flow, nullptr);
}

TEST_F(MacsecManagerTest, addMacsecSecureChannelForNonexistentMacsec) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  EXPECT_THROW(
      saiManagerTable->macsecManager().addMacsecSecureChannel(
          swPort->getID(),
          SAI_MACSEC_DIRECTION_INGRESS,
          utility::packSecureChannelId(remoteSci),
          true),
      FbossError);
}

TEST_F(MacsecManagerTest, addMacsecSecureChannelForNonexistentMacsecPort) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  EXPECT_THROW(
      saiManagerTable->macsecManager().addMacsecSecureChannel(
          swPort->getID(),
          SAI_MACSEC_DIRECTION_INGRESS,
          utility::packSecureChannelId(remoteSci),
          true),
      FbossError);
}

TEST_F(MacsecManagerTest, addDuplicateMacsecSecureChannel) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  saiManagerTable->macsecManager().addMacsecSecureChannel(
      swPort->getID(),
      SAI_MACSEC_DIRECTION_INGRESS,
      utility::packSecureChannelId(remoteSci),
      true);

  EXPECT_THROW(
      saiManagerTable->macsecManager().addMacsecSecureChannel(
          swPort->getID(),
          SAI_MACSEC_DIRECTION_INGRESS,
          utility::packSecureChannelId(remoteSci),
          true),
      FbossError);
}

TEST_F(MacsecManagerTest, removeMacsecSecureChannel) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  saiManagerTable->macsecManager().addMacsecSecureChannel(
      swPort->getID(),
      SAI_MACSEC_DIRECTION_INGRESS,
      utility::packSecureChannelId(remoteSci),
      true);

  auto scHandle = saiManagerTable->macsecManager().getMacsecSecureChannelHandle(
      swPort->getID(),
      utility::packSecureChannelId(remoteSci),
      SAI_MACSEC_DIRECTION_INGRESS);

  CHECK_NE(scHandle, nullptr);

  saiManagerTable->macsecManager().removeMacsecSecureChannel(
      swPort->getID(),
      utility::packSecureChannelId(remoteSci),
      SAI_MACSEC_DIRECTION_INGRESS);

  scHandle = saiManagerTable->macsecManager().getMacsecSecureChannelHandle(
      swPort->getID(),
      utility::packSecureChannelId(remoteSci),
      SAI_MACSEC_DIRECTION_INGRESS);

  CHECK_EQ(scHandle, nullptr);
}

TEST_F(MacsecManagerTest, removeNonexistentMacsecSecureChannel) {
  // When there's no macsec pipeline obj
  EXPECT_THROW(
      saiManagerTable->macsecManager().removeMacsecSecureChannel(
          PortID(p0.id),
          utility::packSecureChannelId(remoteSci),
          SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);

  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  // When there's no macsec port
  EXPECT_THROW(
      saiManagerTable->macsecManager().removeMacsecSecureChannel(
          PortID(p0.id),
          utility::packSecureChannelId(remoteSci),
          SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  // When there's no secure channel
  EXPECT_THROW(
      saiManagerTable->macsecManager().removeMacsecSecureChannel(
          PortID(p0.id),
          utility::packSecureChannelId(remoteSci),
          SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);
}

TEST_F(MacsecManagerTest, addMacsecSecureAssoc) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  saiManagerTable->macsecManager().addMacsecSecureChannel(
      swPort->getID(),
      SAI_MACSEC_DIRECTION_INGRESS,
      utility::packSecureChannelId(remoteSci),
      true);

  saiManagerTable->macsecManager().addMacsecSecureAssoc(
      swPort->getID(),
      utility::packSecureChannelId(remoteSci),
      SAI_MACSEC_DIRECTION_INGRESS,
      *rxSak.assocNum_ref(),
      rxSecureAssocKey,
      kTestSalt,
      rxSecureAssocAuthKey,
      kTestSsci);

  auto secureAssocHandle =
      saiManagerTable->macsecManager().getMacsecSecureAssoc(
          swPort->getID(),
          utility::packSecureChannelId(remoteSci),
          SAI_MACSEC_DIRECTION_INGRESS,
          *rxSak.assocNum_ref());

  CHECK_NE(secureAssocHandle, nullptr);
}

TEST_F(MacsecManagerTest, addMacsecSecureAssocForNonexistentSecureChannel) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);
  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  EXPECT_THROW(
      saiManagerTable->macsecManager().addMacsecSecureAssoc(
          swPort->getID(),
          utility::packSecureChannelId(remoteSci),
          SAI_MACSEC_DIRECTION_INGRESS,
          *rxSak.assocNum_ref(),
          rxSecureAssocKey,
          kTestSalt,
          rxSecureAssocAuthKey,
          kTestSsci),
      FbossError);
}

TEST_F(MacsecManagerTest, addDuplicateMacsecSecureAssoc) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  saiManagerTable->macsecManager().addMacsecSecureChannel(
      swPort->getID(),
      SAI_MACSEC_DIRECTION_INGRESS,
      utility::packSecureChannelId(remoteSci),
      true);

  saiManagerTable->macsecManager().addMacsecSecureAssoc(
      swPort->getID(),
      utility::packSecureChannelId(remoteSci),
      SAI_MACSEC_DIRECTION_INGRESS,
      *rxSak.assocNum_ref(),
      rxSecureAssocKey,
      kTestSalt,
      rxSecureAssocAuthKey,
      kTestSsci);

  EXPECT_THROW(
      saiManagerTable->macsecManager().addMacsecSecureAssoc(
          swPort->getID(),
          utility::packSecureChannelId(remoteSci),
          SAI_MACSEC_DIRECTION_INGRESS,
          *rxSak.assocNum_ref(),
          rxSecureAssocKey,
          kTestSalt,
          rxSecureAssocAuthKey,
          kTestSsci),
      FbossError);
}

TEST_F(MacsecManagerTest, removeMacsecSecureAssoc) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  saiManagerTable->macsecManager().addMacsecPort(
      swPort->getID(), SAI_MACSEC_DIRECTION_INGRESS);

  saiManagerTable->macsecManager().addMacsecSecureChannel(
      swPort->getID(),
      SAI_MACSEC_DIRECTION_INGRESS,
      utility::packSecureChannelId(remoteSci),
      true);

  saiManagerTable->macsecManager().addMacsecSecureAssoc(
      swPort->getID(),
      utility::packSecureChannelId(remoteSci),
      SAI_MACSEC_DIRECTION_INGRESS,
      *rxSak.assocNum_ref(),
      rxSecureAssocKey,
      kTestSalt,
      rxSecureAssocAuthKey,
      kTestSsci);

  auto secureAssocHandle =
      saiManagerTable->macsecManager().getMacsecSecureAssoc(
          swPort->getID(),
          utility::packSecureChannelId(remoteSci),
          SAI_MACSEC_DIRECTION_INGRESS,
          *rxSak.assocNum_ref());

  CHECK_NE(secureAssocHandle, nullptr);

  saiManagerTable->macsecManager().removeMacsecSecureAssoc(
      swPort->getID(),
      utility::packSecureChannelId(remoteSci),
      SAI_MACSEC_DIRECTION_INGRESS,
      *rxSak.assocNum_ref());

  secureAssocHandle = saiManagerTable->macsecManager().getMacsecSecureAssoc(
      swPort->getID(),
      utility::packSecureChannelId(remoteSci),
      SAI_MACSEC_DIRECTION_INGRESS,
      *rxSak.assocNum_ref());

  CHECK_EQ(secureAssocHandle, nullptr);
}

TEST_F(MacsecManagerTest, removeNonexistentMacsecSecureAssoc) {
  EXPECT_THROW(
      saiManagerTable->macsecManager().removeMacsecSecureAssoc(
          PortID(p0.id),
          utility::packSecureChannelId(remoteSci),
          SAI_MACSEC_DIRECTION_INGRESS,
          *rxSak.assocNum_ref()),
      FbossError);
}

TEST_F(MacsecManagerTest, invalidLinePort) {
  // If the linePort doesn't exist, throw an error.
  EXPECT_THROW(
      saiManagerTable->macsecManager().setupMacsec(
          PortID(p0.id), rxSak, remoteSci, SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);
  EXPECT_THROW(
      saiManagerTable->macsecManager().setupMacsec(
          PortID(p0.id), txSak, localSci, SAI_MACSEC_DIRECTION_EGRESS),
      FbossError);
}

TEST_F(MacsecManagerTest, installKeys) {
  std::shared_ptr<Port> swPort = makePort(p0);
  saiManagerTable->portManager().addPort(swPort);

  // Install RX
  saiManagerTable->macsecManager().setupMacsec(
      swPort->getID(), rxSak, remoteSci, SAI_MACSEC_DIRECTION_INGRESS);

  // Install TX
  saiManagerTable->macsecManager().setupMacsec(
      swPort->getID(), txSak, *txSak.sci_ref(), SAI_MACSEC_DIRECTION_EGRESS);

  auto verify = [this](
                    auto direction,
                    auto linePortId,
                    auto secureChannelId,
                    auto assocNum) {
    auto macsecHandle =
        saiManagerTable->macsecManager().getMacsecHandle(direction);
    ASSERT_NE(macsecHandle, nullptr);
    ASSERT_NE(macsecHandle->macsec, nullptr);

    auto macsecPort = macsecHandle->ports.find(linePortId);
    ASSERT_NE(macsecPort, macsecHandle->ports.end());
    ASSERT_NE(macsecPort->second, nullptr);
    ASSERT_NE(macsecPort->second->port, nullptr);

    auto macsecSc = macsecPort->second->secureChannels.find(secureChannelId);
    ASSERT_NE(macsecSc, macsecPort->second->secureChannels.end());
    ASSERT_NE(macsecSc->second, nullptr);
    ASSERT_NE(macsecSc->second->secureChannel, nullptr);
    ASSERT_NE(macsecSc->second->flow, nullptr);

    auto macsecSa = macsecSc->second->secureAssocs.find(assocNum);
    ASSERT_NE(macsecSa, macsecSc->second->secureAssocs.end());
    ASSERT_NE(macsecSa->second, nullptr);

    auto aclName = utility::getAclName(linePortId, direction);
    auto aclTable =
        saiManagerTable->aclTableManager().getAclTableHandle(aclName);
    ASSERT_NE(aclTable, nullptr);

    auto aclEntry =
        saiManagerTable->aclTableManager().getAclEntryHandle(aclTable, 1);
    ASSERT_NE(aclEntry, nullptr);
  };

  verify(
      SAI_MACSEC_DIRECTION_INGRESS,
      swPort->getID(),
      utility::packSecureChannelId(remoteSci),
      *rxSak.assocNum_ref());
  verify(
      SAI_MACSEC_DIRECTION_EGRESS,
      swPort->getID(),
      utility::packSecureChannelId(localSci),
      *txSak.assocNum_ref());
}
} // namespace facebook::fboss
