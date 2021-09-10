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
#include "fboss/agent/platforms/sai/SaiHwPlatform.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/lib/phy/SaiPhyManager.h"
#include "fboss/mka_service/if/gen-cpp2/mka_types.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"

#include <folly/logging/xlog.h>

namespace {
using namespace facebook::fboss;

mka::MKASci makeSci(std::string mac, PortID portId) {
  mka::MKASci sci;
  sci.macAddress_ref() = mac;
  sci.port_ref() = portId;
  return sci;
}

mka::MKASak makeSak(
    mka::MKASci& sci,
    std::string portName,
    std::string keyHex,
    std::string keyIdHex,
    int assocNum) {
  mka::MKASak sak;
  sak.sci_ref() = sci;
  sak.l2Port_ref() = portName;
  sak.keyHex_ref() = keyHex;
  sak.keyIdHex_ref() = keyIdHex;
  sak.assocNum_ref() = assocNum;
  return sak;
}

auto kPortProfile = cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL;
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
      PhyManager* phyManager) {
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
    auto mac = folly::MacAddress(*sci.macAddress_ref());
    auto scIdentifier = MacsecSecureChannelId(mac.u64NBO() | *sci.port_ref());
    auto aclName = folly::to<std::string>(
        "macsec-",
        direction == SAI_MACSEC_DIRECTION_INGRESS ? "ingress" : "egress",
        "-port",
        portId);
    auto assocNum = *sak.assocNum_ref() % 4;
    std::array<uint8_t, 32> secureAssocKey;
    std::copy(
        sak.keyHex_ref()->begin(),
        sak.keyHex_ref()->end(),
        secureAssocKey.data());
    std::array<uint8_t, 16> authKey;
    std::copy(
        sak.keyIdHex_ref()->begin(), sak.keyIdHex_ref()->end(), authKey.data());
    // TODO(ccpowers): These are currently hard coded in the macsecManager
    // they should eventually be made dynamic
    auto shortSecureChannelId = 0x01000000;
    std::array<uint8_t, 12> salt{
        0x9d, 0x00, 0x29, 0x02, 0x48, 0xde, 0x86, 0xa2, 0x1c, 0x66, 0xfa, 0x6d};

    auto portHandle = portManager.getPortHandle(portId);
    auto macsecHandle = macsecManager.getMacsecHandle(direction);
    auto macsecPortHandle =
        macsecManager.getMacsecPortHandle(portId, direction);
    auto macsecSecureChannelHandle = macsecManager.getMacsecSecureChannelHandle(
        portId, scIdentifier, direction);
    auto macsecSecureAssoc = macsecManager.getMacsecSecureAssoc(
        portId, scIdentifier, direction, *sak.assocNum_ref() % 4);

    // Verify all macsec objects were created
    EXPECT_NE(macsecHandle, nullptr);
    EXPECT_NE(macsecHandle->macsec, nullptr);
    EXPECT_NE(macsecPortHandle, nullptr);
    EXPECT_NE(macsecPortHandle->port, nullptr);
    EXPECT_NE(macsecSecureChannelHandle, nullptr);
    EXPECT_NE(macsecSecureChannelHandle->flow, nullptr);
    EXPECT_NE(macsecSecureChannelHandle->secureChannel, nullptr);
    EXPECT_NE(macsecSecureAssoc, nullptr);

    auto saiPortId = portHandle->port->adapterKey();
    auto macsecId = macsecHandle->macsec->adapterKey();
    auto macsecPortId = macsecPortHandle->port->adapterKey();
    auto macsecSecureChannelId =
        macsecSecureChannelHandle->secureChannel->adapterKey();
    auto macsecSecureAssocId = macsecSecureAssoc->adapterKey();
    auto macsecFlowId = macsecSecureChannelHandle->flow->adapterKey();

    auto replayProtectionWindow =
        (direction == SAI_MACSEC_DIRECTION_INGRESS) ? 100 : 0;
    auto secureChannelEnable = (direction == SAI_MACSEC_DIRECTION_EGRESS);

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
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureAssocId, SaiMacsecSATraits::Attributes::AuthKey()),
        authKey);
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
    EXPECT_EQ(
        macsecApi.getAttribute(
            macsecSecureAssocId, SaiMacsecSATraits::Attributes::SAK()),
        secureAssocKey);
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
    auto localSci = makeSci("00:00:00:00:00:00", port);
    auto remoteSci = makeSci("11:11:11:11:11:11", port);
    auto rxSak = makeSak(
        remoteSci,
        *platPort->second.mapping_ref()->name_ref(),
        "01020304050607080910111213141516",
        "0807060504030201",
        0);
    auto txSak = makeSak(
        localSci,
        *platPort->second.mapping_ref()->name_ref(),
        "16151413121110090807060504030201",
        "0102030405060708",
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

} // namespace facebook::fboss
