/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwExternalPhyPortTest.h"

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/lib/phy/SaiPhyManager.h"
#include "fboss/mka_service/if/gen-cpp2/mka_structs_types.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

#include <folly/logging/xlog.h>

namespace {
using namespace facebook::fboss;

mka::MKASci makeSci(std::string mac, PortID portId) {
  mka::MKASci sci;
  sci.macAddress() = mac;
  sci.port() = portId;
  return sci;
}

mka::MKASak makeSak(
    mka::MKASci& sci,
    std::string portName,
    std::string keyHex,
    std::string keyIdHex,
    int assocNum) {
  mka::MKASak sak;
  sak.sci() = sci;
  sak.l2Port() = portName;
  sak.keyHex() = keyHex;
  sak.keyIdHex() = keyIdHex;
  sak.assocNum() = assocNum;
  return sak;
}

} // namespace

namespace facebook::fboss {
class HwMacsecTest : public HwExternalPhyPortTest {
 public:
  const std::vector<phy::ExternalPhy::Feature>& neededFeatures()
      const override {
    static const std::vector<phy::ExternalPhy::Feature> kNeededFeatures = {
        phy::ExternalPhy::Feature::MACSEC};
    return kNeededFeatures;
  }

  void verifyMacsecProgramming(
      PortID portId,
      mka::MKASak& sak,
      mka::MKASci& sci,
      sai_macsec_direction_t direction,
      PhyManager* phyManager,
      bool expectAbsent = false,
      bool expectKeyMismatch = false) {
    if (!getHwQsfpEnsemble()->isSaiPlatform()) {
      throw FbossError("Cannot verify Macsec programming on non-sai platform");
    }

    auto saiPhyManager = static_cast<SaiPhyManager*>(phyManager);
    auto saiApiTable = SaiApiTable::getInstance();
    auto* managerTable =
        static_cast<SaiSwitch*>(
            saiPhyManager->getSaiPlatform(portId)->getHwSwitch())
            ->managerTable();

    auto& macsecApi = saiApiTable->macsecApi();

    auto& portManager = managerTable->portManager();
    auto& macsecManager = managerTable->macsecManager();
    auto& aclManager = managerTable->aclTableManager();

    // Some parameters that we'll verify are set correctly
    auto mac = folly::MacAddress(*sci.macAddress());
    auto scIdentifier = MacsecSecureChannelId(mac.u64NBO() | *sci.port());
    auto aclName = folly::to<std::string>(
        "macsec-",
        direction == SAI_MACSEC_DIRECTION_INGRESS ? "ingress" : "egress",
        "-port",
        portId);
    auto assocNum = *sak.assocNum() % 4;
    std::array<uint8_t, 32> secureAssocKey;
    std::vector<uint8_t> unhexedKey;
    folly::unhexlify(*sak.keyHex(), unhexedKey);
    EXPECT_EQ(unhexedKey.size(), 32);
    std::copy(unhexedKey.begin(), unhexedKey.end(), secureAssocKey.data());
    std::array<uint8_t, 16> authKey;
    std::vector<uint8_t> unhexedKeyId;
    folly::unhexlify(*sak.keyIdHex(), unhexedKeyId);
    EXPECT_EQ(unhexedKeyId.size(), 16);
    std::copy(unhexedKeyId.begin(), unhexedKeyId.end(), authKey.data());
    // TODO(ccpowers): These are currently hard coded in the macsecManager
    // they should eventually be made dynamic
    auto shortSecureChannelId = 0x01000000;
    std::array<uint8_t, 12> salt{
        0x9d, 0x00, 0x29, 0x02, 0x48, 0xde, 0x86, 0xa2, 0x1c, 0x66, 0xfa, 0x6d};

    auto portHandle = portManager.getPortHandle(portId);
    auto macsecHandle = macsecManager.getMacsecHandle(direction);

    SaiMacsecPortHandle* macsecPortHandle{nullptr};
    if (macsecHandle) {
      macsecPortHandle = macsecManager.getMacsecPortHandle(portId, direction);
    }

    SaiMacsecSecureChannelHandle* macsecSecureChannelHandle{nullptr};
    if (macsecPortHandle) {
      macsecSecureChannelHandle = macsecManager.getMacsecSecureChannelHandle(
          portId, scIdentifier, direction);
    }

    SaiMacsecSecureAssoc* macsecSecureAssoc{nullptr};
    if (macsecSecureChannelHandle) {
      macsecSecureAssoc = macsecManager.getMacsecSecureAssoc(
          portId, scIdentifier, direction, *sak.assocNum() % 4);
    }

    // Verify all macsec objects were created/destroyed
    if (!expectAbsent) {
      ASSERT_NE(macsecHandle, nullptr);
      EXPECT_NE(macsecHandle->macsec, nullptr);
      ASSERT_NE(macsecPortHandle, nullptr);
      EXPECT_NE(macsecPortHandle->port, nullptr);
      ASSERT_NE(macsecSecureChannelHandle, nullptr);
      EXPECT_NE(macsecSecureChannelHandle->flow, nullptr);
      EXPECT_NE(macsecSecureChannelHandle->secureChannel, nullptr);
      ASSERT_NE(macsecSecureAssoc, nullptr);
    } else {
      // Check only that the SA is gone. The rest of the objects may and
      // may not be gone, depending on whether we still have other keys
      // there.
      EXPECT_EQ(macsecSecureAssoc, nullptr);
      return;
    }

    auto saiPortId = portHandle->port->adapterKey();
    auto macsecId = macsecHandle->macsec->adapterKey();
    auto macsecPortId = macsecPortHandle->port->adapterKey();
    auto macsecSecureChannelId =
        macsecSecureChannelHandle->secureChannel->adapterKey();
    auto macsecSecureAssocId = macsecSecureAssoc->adapterKey();
    auto macsecFlowId = macsecSecureChannelHandle->flow->adapterKey();

    auto replayProtectionWindow =
        (direction == SAI_MACSEC_DIRECTION_INGRESS) ? 4096 : 0;
    bool secureChannelEnable = true;

    // Verify macsec objects have the correct settings
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecId, SaiMacsecTraits::Attributes::Direction()),
        direction);
    EXPECT_FALSE(macsecApi.getAttribute(
        macsecId, SaiMacsecTraits::Attributes::PhysicalBypass()));

    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecPortId, SaiMacsecPortTraits::Attributes::PortID()),
        saiPortId);
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecPortId, SaiMacsecPortTraits::Attributes::MacsecDirection()),
        direction);

    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureChannelId, SaiMacsecSCTraits::Attributes::SCI()),
        scIdentifier);
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureChannelId,
            SaiMacsecSCTraits::Attributes::MacsecDirection()),
        direction);
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureChannelId, SaiMacsecSCTraits::Attributes::FlowID()),
        macsecFlowId);
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)
    // Assumes we're always using XPN
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureChannelId,
            SaiMacsecSCTraits::Attributes::CipherSuite()),
        SAI_MACSEC_CIPHER_SUITE_GCM_AES_XPN_256);
#endif
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureChannelId, SaiMacsecSCTraits::Attributes::SCIEnable()),
        secureChannelEnable);
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureChannelId,
            SaiMacsecSCTraits::Attributes::ReplayProtectionEnable()),
        direction == SAI_MACSEC_DIRECTION_INGRESS);
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureChannelId,
            SaiMacsecSCTraits::Attributes::ReplayProtectionWindow()),
        replayProtectionWindow);
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureChannelId,
            SaiMacsecSCTraits::Attributes::SectagOffset()),
        12);

    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureAssocId, SaiMacsecSATraits::Attributes::SCID()),
        macsecSecureChannelId);
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureAssocId, SaiMacsecSATraits::Attributes::AssocNum()),
        assocNum);
    if (!expectKeyMismatch) {
      EXPECT_EQ(
          macsecApi.getAttribute(
              macsecSecureAssocId, SaiMacsecSATraits::Attributes::AuthKey()),
          authKey);
    } else {
      EXPECT_NE(
          macsecApi.getAttribute(
              macsecSecureAssocId, SaiMacsecSATraits::Attributes::AuthKey()),
          authKey);
    }
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureAssocId,
            SaiMacsecSATraits::Attributes::MacsecDirection()),
        direction);
#if SAI_API_VERSION >= SAI_VERSION(1, 7, 1)

    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureAssocId, SaiMacsecSATraits::Attributes::SSCI()),
        shortSecureChannelId);
#endif
    if (!expectKeyMismatch) {
      EXPECT_EQ(
          macsecApi.getAttribute(
              macsecSecureAssocId, SaiMacsecSATraits::Attributes::SAK()),
          secureAssocKey);
    } else {
      EXPECT_NE(
          macsecApi.getAttribute(
              macsecSecureAssocId, SaiMacsecSATraits::Attributes::SAK()),
          secureAssocKey);
    }

    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureAssocId, SaiMacsecSATraits::Attributes::Salt()),
        salt);

    // Verify ACLs are set up correctly
    auto aclTableHandle = aclManager.getAclTableHandle(aclName);
    EXPECT_NE(aclTableHandle, nullptr);
    auto aclEntryHandle = aclManager.getAclEntryHandle(aclTableHandle, 1);
    EXPECT_NE(aclEntryHandle, nullptr);

    // TODO(ccpowers):
    // * verify that we set the minimumXpn correctly
    // * verify that we set ingressmacsecAcl and egressMacsecAcl correctly
    // * verify that the acl entry is actually valid
  }
  void rotateKeysMultiple(bool circleThroughAN = true);

  void verifyMacsecAclSetup(
      PortID portId,
      sai_macsec_direction_t direction,
      PhyManager* phyManager,
      bool macsecDesired = true,
      bool dropUnencrypted = false) {
    if (!getHwQsfpEnsemble()->isSaiPlatform()) {
      throw FbossError("Cannot verify Macsec programming on non-sai platform");
    }

    auto saiPhyManager = static_cast<SaiPhyManager*>(phyManager);
    auto* managerTable =
        static_cast<SaiSwitch*>(
            saiPhyManager->getSaiPlatform(portId)->getHwSwitch())
            ->managerTable();

    auto& aclManager = managerTable->aclTableManager();

    // Get Macsec ACL table
    auto aclName = folly::to<std::string>(
        "macsec-",
        direction == SAI_MACSEC_DIRECTION_INGRESS ? "ingress" : "egress",
        "-port",
        portId);
    auto aclTableHandle = aclManager.getAclTableHandle(aclName);

    // If macsecDesired is false then ACL table should not exist
    if (!macsecDesired) {
      EXPECT_EQ(aclTableHandle, nullptr);
      return;
    } else {
      EXPECT_NE(aclTableHandle, nullptr);
    }

    // Check the default data packet ACL rule action with dropUnencrypted value
    auto aclEntryHandle = aclManager.getAclEntryHandle(aclTableHandle, 4);
    EXPECT_NE(aclEntryHandle, nullptr);

    auto aclAttrs = aclEntryHandle->aclEntry->attributes();
    auto pktAction = GET_OPT_ATTR(AclEntry, ActionPacketAction, aclAttrs);

    if (dropUnencrypted) {
      EXPECT_EQ(pktAction.getData(), SAI_PACKET_ACTION_DROP);
    } else {
      EXPECT_EQ(pktAction.getData(), SAI_PACKET_ACTION_FORWARD);
    }
  }

  SaiPhyManager* phyManager;
};

TEST_F(HwMacsecTest, installRemoveKeys) {
  auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
  const auto& platPorts =
      getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
  for (const auto& [port, profile] : findAvailableXphyPorts()) {
    auto platPort = platPorts.find(port);
    CHECK(platPort != platPorts.end())
        << " Could not find platform port with ID " << port;
    auto macGen = facebook::fboss::utility::MacAddressGenerator();
    auto sakKeyGen = facebook::fboss::utility::SakKeyHexGenerator();
    auto sakKeyIdGen = facebook::fboss::utility::SakKeyIdHexGenerator();
    auto localSci = makeSci(macGen.getNext().toString(), port);
    auto remoteSci = makeSci(macGen.getNext().toString(), port);
    auto rxSak = makeSak(
        remoteSci,
        *platPort->second.mapping()->name(),
        sakKeyGen.getNext(),
        sakKeyIdGen.getNext(),
        0);
    auto txSak = makeSak(
        localSci,
        *platPort->second.mapping()->name(),
        sakKeyGen.getNext(),
        sakKeyIdGen.getNext(),
        0);
    wedgeManager->programXphyPort(port, profile);

    XLOG(INFO) << "installKeys: Installing Macsec TX for port " << port;
    phyManager->sakInstallTx(txSak);

    XLOG(INFO) << "installKeys: Verifying Macsec TX for port " << port;
    verifyMacsecProgramming(
        port, txSak, localSci, SAI_MACSEC_DIRECTION_EGRESS, phyManager);

    XLOG(INFO) << "installKeys: Installing Macsec RX for port " << port;
    phyManager->sakInstallRx(rxSak, remoteSci);

    XLOG(INFO) << "installKeys: Verifying Macsec RX for port " << port;
    verifyMacsecProgramming(
        port, rxSak, remoteSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager);
    // Delete keys
    phyManager->sakDeleteRx(rxSak, remoteSci);
    phyManager->sakDelete(txSak);
  }
}

TEST_F(HwMacsecTest, rotateRxKeys) {
  auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
  const auto& platPorts =
      getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
  for (const auto& [port, profile] : findAvailableXphyPorts()) {
    auto platPort = platPorts.find(port);
    CHECK(platPort != platPorts.end())
        << " Could not find platform port with ID " << port;
    auto macGen = facebook::fboss::utility::MacAddressGenerator();
    auto sakKeyGen = facebook::fboss::utility::SakKeyHexGenerator();
    auto sakKeyIdGen = facebook::fboss::utility::SakKeyIdHexGenerator();
    auto remoteSci = makeSci(macGen.getNext().toString(), port);
    auto rxSak = makeSak(
        remoteSci,
        *platPort->second.mapping()->name(),
        sakKeyGen.getNext(),
        sakKeyIdGen.getNext(),
        0);
    // 2nd key with new association number
    auto rxSak2 = makeSak(
        remoteSci,
        *platPort->second.mapping()->name(),
        sakKeyGen.getNext(),
        sakKeyIdGen.getNext(),
        1);

    wedgeManager->programXphyPort(port, profile);

    XLOG(INFO) << "rotateRxKeys: Installing Macsec RX for port " << port;
    phyManager->sakInstallRx(rxSak, remoteSci);

    XLOG(INFO) << "rotateRxKeys: Verifying Macsec RX for port " << port;
    verifyMacsecProgramming(
        port, rxSak, remoteSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager);

    XLOG(INFO) << "rotateRxKeys: Installing new Macsec RX for port " << port;
    phyManager->sakInstallRx(rxSak2, remoteSci);

    XLOG(INFO) << "rotateRxKeys: Verifying new and old Macsec RX for port "
               << port;
    verifyMacsecProgramming(
        port, rxSak, remoteSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager);
    verifyMacsecProgramming(
        port, rxSak2, remoteSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager);

    XLOG(INFO) << "rotateRxKeys: Removing old Macsec RX for port " << port;
    phyManager->sakDeleteRx(rxSak, remoteSci);

    XLOG(INFO) << "rotateRxKeys: Verifying removal of old Macsec RX for port "
               << port;
    verifyMacsecProgramming(
        port, rxSak, remoteSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager, true);
    verifyMacsecProgramming(
        port, rxSak2, remoteSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager);

    // Delete keys
    phyManager->sakDeleteRx(rxSak2, remoteSci);
  }
}

// Verify that the RX SAK APIs are idempotent because MKA service may need
// to retry in some cases.
TEST_F(HwMacsecTest, idempotentRx) {
  auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
  const auto& platPorts =
      getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
  for (const auto& [port, profile] : findAvailableXphyPorts()) {
    auto platPort = platPorts.find(port);
    CHECK(platPort != platPorts.end())
        << " Could not find platform port with ID " << port;
    auto macGen = facebook::fboss::utility::MacAddressGenerator();
    auto sakKeyGen = facebook::fboss::utility::SakKeyHexGenerator();
    auto sakKeyIdGen = facebook::fboss::utility::SakKeyIdHexGenerator();
    auto remoteSci = makeSci(macGen.getNext().toString(), port);
    auto rxSak = makeSak(
        remoteSci,
        *platPort->second.mapping()->name(),
        sakKeyGen.getNext(),
        sakKeyIdGen.getNext(),
        0);

    wedgeManager->programXphyPort(port, profile);

    XLOG(INFO) << "idempotentRx: 2x installing RX SAK for port " << port;
    phyManager->sakInstallRx(rxSak, remoteSci);
    phyManager->sakInstallRx(rxSak, remoteSci);

    XLOG(INFO) << "idempotentRx: Verifying RX SAK for port " << port;
    verifyMacsecProgramming(
        port, rxSak, remoteSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager);

    XLOG(INFO) << "idempotentRx: Removing RX SAK for port " << port;
    phyManager->sakDeleteRx(rxSak, remoteSci);

    XLOG(INFO) << "idempotentRx: Verifying removal of RX SAK for port " << port;
    verifyMacsecProgramming(
        port, rxSak, remoteSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager, true);

    XLOG(INFO) << "idempotentRx: Again removing RX SAK for port " << port;
    phyManager->sakDeleteRx(rxSak, remoteSci);

    XLOG(INFO) << "idempotentRx: Again verifying removal of RX SAK for port "
               << port;
    verifyMacsecProgramming(
        port, rxSak, remoteSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager, true);
  }
}

/*
 * updateRxKeys()
 *
 * This test will check Rx SAK updates:
 * 1. Install Rx SAK1 (SC=X, AN=1, Key=Z), Rx SAK2 (SC=X, AN=2, Key=Z). Verify
 *    SAK1 and SAK2 are programmed
 * 2. Install Rx SAK3 (SC=X, AN=1, Key=W), Verify SAK3 is programmed, SAK1 is
 *    removed, SAK2 is untouched
 */
TEST_F(HwMacsecTest, updateRxKeys) {
  auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
  const auto& platPorts =
      getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
  for (const auto& [port, profile] : findAvailableXphyPorts()) {
    auto platPort = platPorts.find(port);
    CHECK(platPort != platPorts.end())
        << " Could not find platform port with ID " << port;

    wedgeManager->programXphyPort(port, profile);

    auto macGen = facebook::fboss::utility::MacAddressGenerator();
    auto sakKeyGen = facebook::fboss::utility::SakKeyHexGenerator();
    auto sakKeyIdGen = facebook::fboss::utility::SakKeyIdHexGenerator();

    auto localSci = makeSci(macGen.getNext().toString(), port);

    // Install Rx SAK1 (SC=X, AN=1, Key=Z), Rx SAK2 (SC=X, AN=2, Key=Z)
    auto sakKey1 = sakKeyGen.getNext();
    auto sakKeyId1 = sakKeyIdGen.getNext();
    auto rxSak1 = makeSak(
        localSci, *platPort->second.mapping()->name(), sakKey1, sakKeyId1, 1);
    XLOG(INFO) << "updateRxKeys: Installing RX SAK1 for port " << port;
    phyManager->sakInstallRx(rxSak1, localSci);

    auto rxSak2 = makeSak(
        localSci, *platPort->second.mapping()->name(), sakKey1, sakKeyId1, 2);
    XLOG(INFO) << "updateRxKeys: Installing RX SAK2 for port " << port;
    phyManager->sakInstallRx(rxSak2, localSci);

    // Verify programming of Rx SAK1, SAK2
    XLOG(INFO) << "updateRxKeys: Verifying RX SAK1 present for port " << port;
    verifyMacsecProgramming(
        port, rxSak1, localSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager);
    XLOG(INFO) << "updateRxKeys: Verifying RX SAK2 present for port " << port;
    verifyMacsecProgramming(
        port, rxSak2, localSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager);

    // Install Rx SAK3 (SC=X, AN=1, Key=W) and verify SAK3 programming
    auto sakKey3 = sakKeyGen.getNext();
    auto sakKeyId3 = sakKeyIdGen.getNext();
    auto rxSak3 = makeSak(
        localSci, *platPort->second.mapping()->name(), sakKey3, sakKeyId3, 1);
    XLOG(INFO) << "updateRxKeys: Installing RX SAK3 for port " << port;
    phyManager->sakInstallRx(rxSak3, localSci);
    XLOG(INFO) << "updateRxKeys: Verifying RX SAK3 present for port " << port;
    verifyMacsecProgramming(
        port, rxSak3, localSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager);

    // Verify SAK1 is not present
    XLOG(INFO) << "updateRxKeys: Verifying no RX SAK1 for port " << port;
    verifyMacsecProgramming(
        port,
        rxSak1,
        localSci,
        SAI_MACSEC_DIRECTION_INGRESS,
        phyManager,
        false,
        true);

    // Verify SAK2 is still present
    XLOG(INFO) << "updateRxKeys: Verifying RX SAK2 present for port " << port;
    verifyMacsecProgramming(
        port, rxSak2, localSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager);

    // Cleanup - Delete SAK2 and SAK3
    phyManager->sakDeleteRx(rxSak2, localSci);
    phyManager->sakDeleteRx(rxSak3, localSci);
  }
}

/*
 * updateTxKeys()
 *
 * This test will check Rx SAK updates:
 * 1. Install Tx SAK1 (SC=X, AN=1, Key=Z), Tx SAK2 (SC=X, AN=2, Key=Z). Verify
 *    SAK2 is programmed
 * 2. Install Tx SAK3 (SC=X, AN=2, Key=W), Verify SAK3 is programmed
 */
TEST_F(HwMacsecTest, updateTxKeys) {
  auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
  const auto& platPorts =
      getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
  for (const auto& [port, profile] : findAvailableXphyPorts()) {
    auto platPort = platPorts.find(port);
    CHECK(platPort != platPorts.end())
        << " Could not find platform port with ID " << port;

    wedgeManager->programXphyPort(port, profile);

    auto macGen = facebook::fboss::utility::MacAddressGenerator();
    auto sakKeyGen = facebook::fboss::utility::SakKeyHexGenerator();
    auto sakKeyIdGen = facebook::fboss::utility::SakKeyIdHexGenerator();

    auto localSci = makeSci(macGen.getNext().toString(), port);

    // Install Tx SAK1 (SC=X, AN=1, Key=Z), Tx SAK2 (SC=X, AN=2, Key=Z)
    auto sakKey1 = sakKeyGen.getNext();
    auto sakKeyId1 = sakKeyIdGen.getNext();
    auto txSak1 = makeSak(
        localSci, *platPort->second.mapping()->name(), sakKey1, sakKeyId1, 1);
    XLOG(INFO) << "updateTxKeys: Installing TX SAK1 for port " << port;
    phyManager->sakInstallTx(txSak1);

    auto txSak2 = makeSak(
        localSci, *platPort->second.mapping()->name(), sakKey1, sakKeyId1, 2);
    XLOG(INFO) << "updateTxKeys: Installing TX SAK2 for port " << port;
    phyManager->sakInstallTx(txSak2);

    // Verify programming of Tx SAK2
    XLOG(INFO) << "updateTxKeys: Verifying TX SAK2 for port " << port;
    verifyMacsecProgramming(
        port, txSak2, localSci, SAI_MACSEC_DIRECTION_EGRESS, phyManager);

    // Install Tx SAK3 (SC=X, AN=2, Key=W), Verify SAK3 is programmed
    auto sakKey3 = sakKeyGen.getNext();
    auto sakKeyId3 = sakKeyIdGen.getNext();
    auto txSak3 = makeSak(
        localSci, *platPort->second.mapping()->name(), sakKey3, sakKeyId3, 2);
    XLOG(INFO) << "updateTxKeys: Installing TX SAK3 for port " << port;
    phyManager->sakInstallTx(txSak3);
    XLOG(INFO) << "updateTxKeys: Verifying TX SAK3 for port " << port;
    verifyMacsecProgramming(
        port, txSak3, localSci, SAI_MACSEC_DIRECTION_EGRESS, phyManager);

    // Cleanup - Delete SAK1 and SAK3
    phyManager->sakDelete(txSak1);
    phyManager->sakDelete(txSak3);
  }
}

TEST_F(HwMacsecTest, cleanupMacsec) {
  auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
  const auto& platPorts =
      getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
  for (const auto& [port, profile] : findAvailableXphyPorts()) {
    auto platPort = platPorts.find(port);
    CHECK(platPort != platPorts.end())
        << " Could not find platform port with ID " << port;

    auto macGen = facebook::fboss::utility::MacAddressGenerator();
    auto sakKeyGen = facebook::fboss::utility::SakKeyHexGenerator();
    auto sakKeyIdGen = facebook::fboss::utility::SakKeyIdHexGenerator();

    auto localSci = makeSci(macGen.getNext().toString(), port);
    auto remoteSci = makeSci(macGen.getNext().toString(), port);
    auto sakKey1 = sakKeyGen.getNext();
    auto sakKeyId1 = sakKeyIdGen.getNext();
    auto sakKey2 = sakKeyGen.getNext();
    auto sakKeyId2 = sakKeyIdGen.getNext();
    auto rxSak = makeSak(
        remoteSci, *platPort->second.mapping()->name(), sakKey1, sakKeyId1, 1);
    auto txSak = makeSak(
        localSci, *platPort->second.mapping()->name(), sakKey2, sakKeyId2, 0);

    wedgeManager->programXphyPort(port, profile);

    // Set the MacsecDesired=True
    phyManager->setupMacsecState(
        {*platPort->second.mapping()->name()}, true, true);

    // Install and verify the Macsec Tx Key
    XLOG(INFO) << "Install and verify Macsec TX key for port " << port;
    phyManager->sakInstallTx(txSak);
    verifyMacsecProgramming(
        port, txSak, localSci, SAI_MACSEC_DIRECTION_EGRESS, phyManager);

    // Install and verify Macsec Rx Key
    XLOG(INFO) << "Install and verify Macsec RX key for port " << port;
    phyManager->sakInstallRx(rxSak, remoteSci);
    verifyMacsecProgramming(
        port, rxSak, remoteSci, SAI_MACSEC_DIRECTION_INGRESS, phyManager);

    // Set MacsecDesired=False which will cleanup Macsec on the port
    phyManager->setupMacsecState(
        {*platPort->second.mapping()->name()}, false, false);

    // Verify no Macsec keys exits on Tx and Rx ports
    XLOG(INFO) << "Verify Macsec TX and RX keys got removed for port " << port;
    verifyMacsecProgramming(
        port,
        txSak,
        localSci,
        SAI_MACSEC_DIRECTION_EGRESS,
        phyManager,
        true,
        true);
    verifyMacsecProgramming(
        port,
        rxSak,
        remoteSci,
        SAI_MACSEC_DIRECTION_INGRESS,
        phyManager,
        true,
        true);
  }
}

TEST_F(HwMacsecTest, verifyMacsecAclStates) {
  auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
  const auto& platPorts =
      getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
  for (const auto& [port, profile] : findAvailableXphyPorts()) {
    auto platPort = platPorts.find(port);
    CHECK(platPort != platPorts.end())
        << " Could not find platform port with ID " << port;

    auto macGen = facebook::fboss::utility::MacAddressGenerator();
    auto sakKeyGen = facebook::fboss::utility::SakKeyHexGenerator();
    auto sakKeyIdGen = facebook::fboss::utility::SakKeyIdHexGenerator();

    auto localSci = makeSci(macGen.getNext().toString(), port);
    auto remoteSci = makeSci(macGen.getNext().toString(), port);
    auto sakKey1 = sakKeyGen.getNext();
    auto sakKeyId1 = sakKeyIdGen.getNext();
    auto sakKey2 = sakKeyGen.getNext();
    auto sakKeyId2 = sakKeyIdGen.getNext();
    auto rxSak = makeSak(
        remoteSci, *platPort->second.mapping()->name(), sakKey1, sakKeyId1, 1);
    auto txSak = makeSak(
        localSci, *platPort->second.mapping()->name(), sakKey2, sakKeyId2, 0);

    wedgeManager->programXphyPort(port, profile);

    // Verify Macsec state does not exists
    phyManager->setupMacsecState(
        {*platPort->second.mapping()->name()}, false, false);
    XLOG(INFO)
        << "Verify Macsec ACL does not exist by default after init for port "
        << port;
    verifyMacsecAclSetup(
        port, SAI_MACSEC_DIRECTION_EGRESS, phyManager, false, false);
    verifyMacsecAclSetup(
        port, SAI_MACSEC_DIRECTION_INGRESS, phyManager, false, false);

    // Set the MacsecDesired=True, dropUnencrypted=True and verify ACL
    XLOG(INFO)
        << "setupMacsecState with MacsecDesired=True and dropUnencrypted=True and verify for port "
        << port;
    phyManager->setupMacsecState(
        {*platPort->second.mapping()->name()}, true, true);
    verifyMacsecAclSetup(
        port, SAI_MACSEC_DIRECTION_EGRESS, phyManager, true, true);
    verifyMacsecAclSetup(
        port, SAI_MACSEC_DIRECTION_INGRESS, phyManager, true, true);

    // Install the Rx and Tx key with default dropUnencrypted=True and then
    // verify ACL change from Drop to Forward
    XLOG(INFO) << "Install Tx and Rx Macsec key for port " << port;
    phyManager->sakInstallTx(txSak);
    phyManager->sakInstallRx(rxSak, remoteSci);

    XLOG(INFO) << "Verify dropUnencrypted=True for the port " << port;
    verifyMacsecAclSetup(
        port, SAI_MACSEC_DIRECTION_EGRESS, phyManager, true, true);
    verifyMacsecAclSetup(
        port, SAI_MACSEC_DIRECTION_INGRESS, phyManager, true, true);

    // Set dropUnencrypted=False and verify again
    XLOG(INFO)
        << "setupMacsecState with MacsecDesired=True and dropUnencrypted=False and verify for port "
        << port;
    phyManager->setupMacsecState(
        {*platPort->second.mapping()->name()}, true, false);
    verifyMacsecAclSetup(
        port, SAI_MACSEC_DIRECTION_EGRESS, phyManager, true, false);
    verifyMacsecAclSetup(
        port, SAI_MACSEC_DIRECTION_INGRESS, phyManager, true, false);

    // Cleanup the Macsec state
    phyManager->setupMacsecState(
        {*platPort->second.mapping()->name()}, false, false);
    verifyMacsecAclSetup(
        port, SAI_MACSEC_DIRECTION_EGRESS, phyManager, false, false);
    verifyMacsecAclSetup(
        port, SAI_MACSEC_DIRECTION_INGRESS, phyManager, false, false);
  }
}

void HwMacsecTest::rotateKeysMultiple(bool circleThroughAN) {
  auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
  const auto& platPorts =
      getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
  auto macGen = facebook::fboss::utility::MacAddressGenerator();
  auto sakKeyGen = facebook::fboss::utility::SakKeyHexGenerator();
  auto sakKeyIdGen = facebook::fboss::utility::SakKeyIdHexGenerator();
  for (const auto& [port, profile] : findAvailableXphyPorts()) {
    auto platPort = platPorts.find(port);
    CHECK(platPort != platPorts.end())
        << " Could not find platform port with ID " << port;
    auto sci = makeSci(macGen.getNext().toString(), port);
    wedgeManager->programXphyPort(port, profile);
    auto installKeys = [=, port = port](auto direction) mutable {
      std::optional<mka::MKASak> prevSak;
      bool isIngress = direction == SAI_MACSEC_DIRECTION_INGRESS;
      // 5 rotations to overflow 2 bit AN space
      for (auto i = 0; i < 5; ++i) {
        XLOG(INFO) << " Iteration: " << i << " for dir "
                   << (isIngress ? "ingress" : "egress");
        auto an = circleThroughAN ? i % 4 : 0;
        auto sak = makeSak(
            sci,
            *platPort->second.mapping()->name(),
            sakKeyGen.getNext(),
            sakKeyIdGen.getNext(),
            an);
        isIngress ? phyManager->sakInstallRx(sak, sci)
                  : phyManager->sakInstallTx(sak);

        verifyMacsecProgramming(port, sak, sci, direction, phyManager);
        if (prevSak) {
          XLOG(INFO) << "Verifying removal of old SAK for port " << port;
          bool expectAbsent = !isIngress && circleThroughAN;
          verifyMacsecProgramming(
              port,
              *prevSak,
              sci,
              direction,
              phyManager,
              expectAbsent,
              // When circling through AN, if the key is not absent (RX)
              // it will still match in HW, since the key with old AN
              // is still in HW
              !circleThroughAN /*expect key mismatch*/);
        }
        prevSak = sak;
      }
      // Delete keys
      isIngress ? phyManager->sakDeleteRx(*prevSak, sci)
                : phyManager->sakDelete(*prevSak);
      XLOG(INFO) << "Verifying removal of old SAK for port " << port;
      verifyMacsecProgramming(
          port, *prevSak, sci, direction, phyManager, true /*expect absent*/);
    };
    installKeys(SAI_MACSEC_DIRECTION_INGRESS);
    installKeys(SAI_MACSEC_DIRECTION_EGRESS);
  }
}

TEST_F(HwMacsecTest, rotateKeysSameAN) {
  rotateKeysMultiple(false);
}
TEST_F(HwMacsecTest, rotateKeysDifferentAN) {
  rotateKeysMultiple(true);
}
} // namespace facebook::fboss
