/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/phy/PhyManager.h"
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
class HwMacsecTest : public HwTest {
 public:
  std::vector<std::pair<PortID, cfg::PortProfileID>> findAvailablePorts() {
    auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
    const auto& ports = utility::findAvailablePorts(getHwQsfpEnsemble());
    std::vector<std::pair<PortID, cfg::PortProfileID>> macsecSupportedPorts;
    // Check whether the ExternalPhy of such xphy port support macsec feature
    for (auto& [port, profile] : ports.xphyPorts) {
      auto* xphy = phyManager->getExternalPhy(port);
      if (xphy->isSupported(phy::ExternalPhy::Feature::MACSEC)) {
        macsecSupportedPorts.push_back(std::make_pair(port, profile));
      }
    }
    CHECK(!macsecSupportedPorts.empty())
        << "Can't find Macsec-enabled xphy ports";
    return macsecSupportedPorts;
  }
};

TEST_F(HwMacsecTest, installKeys) {
  auto* wedgeManager = getHwQsfpEnsemble()->getWedgeManager();
  auto* phyManager = getHwQsfpEnsemble()->getPhyManager();
  const auto& platPorts =
      getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
  for (const auto& [port, profile] : findAvailablePorts()) {
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
    XLOG(INFO) << "installKeys: Installing Macsec RX for port " << port;
    phyManager->sakInstallRx(rxSak, remoteSci);
  }

  // TODO(ccpowers): verify that the macsec encryption actually works
}

} // namespace facebook::fboss
