/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <fboss/mka_service/if/gen-cpp2/mka_types.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/phy/PhyManager.h"
#include "fboss/lib/phy/facebook/credo/elbert/ElbertPhyManager.h"
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/HwTest.h"
#include "fboss/qsfp_service/util/MacsecUtils.h"

namespace {
auto kPortProfile =
    facebook::fboss::cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL;
} // namespace

namespace facebook::fboss {
class HwMacsecTest : public HwTest {
 public:
  void setupPort(PortID port) {
    XLOG(INFO) << "setupPort: programming port " << port;
    const auto& platformPorts =
        getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
    auto iter = platformPorts.find(port);
    CHECK(iter != platformPorts.end());
    getHwQsfpEnsemble()->getWedgeManager()->programXphyPort(port, kPortProfile);
  }
};

TEST_F(HwMacsecTest, installKeys) {
  auto phyManager = getHwQsfpEnsemble()->getPhyManager();
  auto ports = utility::findAvailablePorts(getHwQsfpEnsemble(), kPortProfile);
  auto macsecPorts = phyManager->getMacsecCapablePorts();
  std::vector<PortID> testPorts;

  // intersect xphyPorts & macsecPorts
  std::sort(ports.xphyPorts.begin(), ports.xphyPorts.end());
  std::sort(macsecPorts.begin(), macsecPorts.end());
  std::set_intersection(
      ports.xphyPorts.begin(),
      ports.xphyPorts.end(),
      macsecPorts.begin(),
      macsecPorts.end(),
      back_inserter(testPorts));

  ASSERT_GE(testPorts.size(), 0)
      << "Macsec-enabled xphy ports found supporting the profile "
      << apache::thrift::util::enumNameSafe(kPortProfile) << ". Skipping test.";

  auto platPorts =
      getHwQsfpEnsemble()->getPlatformMapping()->getPlatformPorts();
  for (const auto& port : testPorts) {
    auto platPort = platPorts.find(port);
    CHECK(platPort != platPorts.end())
        << " Could not find platform port with ID " << port;
    auto localSci = utility::makeSci("00:00:00:00:00:00", port);
    auto remoteSci = utility::makeSci("11:11:11:11:11:11", port);
    auto rxSak = utility::makeSak(
        remoteSci,
        *platPort->second.mapping_ref()->name_ref(),
        "01020304050607080910111213141516",
        "0807060504030201",
        0);
    auto txSak = utility::makeSak(
        localSci,
        *platPort->second.mapping_ref()->name_ref(),
        "16151413121110090807060504030201",
        "0102030405060708",
        0);
    setupPort(port);

    XLOG(INFO) << "installKeys: Installing Macsec TX for port " << port;
    phyManager->sakInstallTx(txSak);
    XLOG(INFO) << "installKeys: Installing Macsec RX for port " << port;
    phyManager->sakInstallRx(rxSak, remoteSci);
  }

  // TODO(ccpowers): verify that the macsec encryption actually works
}

} // namespace facebook::fboss
