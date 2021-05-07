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
#include "fboss/agent/hw/sai/switch/SaiMacsecManager.h"
#include "fboss/agent/hw/sai/switch/tests/ManagerTestBase.h"
#include "fboss/agent/types.h"
#include "fboss/mka_service/if/gen-cpp2/mka_types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

namespace {
mka::MKASci makeSci(std::string mac, PortID portId) {
  mka::MKASci sci;
  sci.macAddress_ref() = mac;
  sci.port_ref() = portId;
  return sci;
}

mka::MKASak makeSak(
    mka::MKASci& sci,
    PortID portId,
    std::string keyHex,
    std::string keyIdHex,
    int assocNum) {
  mka::MKASak sak;
  sak.sci_ref() = sci;
  sak.l2Port_ref() = folly::to<std::string>(portId);
  sak.keyHex_ref() = keyHex;
  sak.keyIdHex_ref() = keyIdHex;
  sak.assocNum_ref() = assocNum;
  return sak;
}
} // namespace

class MacsecManagerTest : public ManagerTestBase {
 public:
  void SetUp() override {
    setupStage = SetupStage::BLANK;
    ManagerTestBase::SetUp();
    p0 = testInterfaces[0].remoteHosts[0].port;
    p1 = testInterfaces[1].remoteHosts[0].port;

    localSci = makeSci("00:00:00:00:00:00", PortID(p0.id));
    remoteSci = makeSci("11:11:11:11:11:11", PortID(p1.id));
    rxSak = makeSak(
        remoteSci,
        PortID(p0.id),
        "01020304050607080910111213141516",
        "0807060504030201",
        0);
    txSak = makeSak(
        localSci,
        PortID(p0.id),
        "16151413121110090807060504030201",
        "0102030405060708",
        0);
  }

  TestPort p0;
  TestPort p1;

  mka::MKASak rxSak;
  mka::MKASak txSak;
  mka::MKASci localSci;
  mka::MKASci remoteSci;
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

TEST_F(MacsecManagerTest, addMacsecFlow) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_EGRESS, false);

  saiManagerTable->macsecManager().addMacsecFlow(SAI_MACSEC_DIRECTION_INGRESS);
  saiManagerTable->macsecManager().addMacsecFlow(SAI_MACSEC_DIRECTION_EGRESS);

  auto ingressFlow = saiManagerTable->macsecManager().getMacsecFlow(
      SAI_MACSEC_DIRECTION_INGRESS);
  auto egressFlow = saiManagerTable->macsecManager().getMacsecFlow(
      SAI_MACSEC_DIRECTION_EGRESS);

  CHECK_NE(ingressFlow, nullptr);
  CHECK_NE(egressFlow, nullptr);
}

TEST_F(MacsecManagerTest, addMacsecFlowForNonexistentMacsec) {
  EXPECT_THROW(
      saiManagerTable->macsecManager().addMacsecFlow(
          SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);
}

TEST_F(MacsecManagerTest, addDuplicateMacsecFlow) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  saiManagerTable->macsecManager().addMacsecFlow(SAI_MACSEC_DIRECTION_INGRESS);
  EXPECT_THROW(
      saiManagerTable->macsecManager().addMacsecFlow(
          SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);
}

TEST_F(MacsecManagerTest, removeMacsecFlow) {
  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  saiManagerTable->macsecManager().addMacsecFlow(SAI_MACSEC_DIRECTION_INGRESS);

  auto ingressFlow = saiManagerTable->macsecManager().getMacsecFlow(
      SAI_MACSEC_DIRECTION_INGRESS);

  CHECK_NE(ingressFlow, nullptr);

  saiManagerTable->macsecManager().removeMacsecFlow(
      SAI_MACSEC_DIRECTION_INGRESS);

  ingressFlow = saiManagerTable->macsecManager().getMacsecFlow(
      SAI_MACSEC_DIRECTION_INGRESS);
  CHECK_EQ(ingressFlow, nullptr);
}

TEST_F(MacsecManagerTest, removeNonexistentMacsecFlow) {
  // When there's no macsec pipeline obj
  EXPECT_THROW(
      saiManagerTable->macsecManager().removeMacsecFlow(
          SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);

  saiManagerTable->macsecManager().addMacsec(
      SAI_MACSEC_DIRECTION_INGRESS, false);

  // When there's a macsec pipeline, but no macsecFlow
  EXPECT_THROW(
      saiManagerTable->macsecManager().removeMacsecFlow(
          SAI_MACSEC_DIRECTION_INGRESS),
      FbossError);
}
