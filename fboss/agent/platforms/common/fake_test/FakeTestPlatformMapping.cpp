/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/common/fake_test/FakeTestPlatformMapping.h"

#include <folly/Format.h>
#include "fboss/agent/FbossError.h"

#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace {
using namespace facebook::fboss;
static const std::unordered_map<int, std::vector<cfg::PortProfileID>>
    k1PortProfilesInGroup = {
        {0,
         {
             cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL,
         }}};

static const std::unordered_map<int, std::vector<cfg::PortProfileID>>
    k4PortProfilesInGroup = {
        {0,
         {
             cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL,
             cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER,
             cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
        {1,
         {
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
        {2,
         {
             cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER,
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
        {3,
         {
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
};

static const std::unordered_map<int, std::vector<cfg::PortProfileID>>
    k4PortProfilesInGroupForMultiTcvr = {
        {0,
         {
             cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER,
             cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_COPPER,
             cfg::PortProfileID::PROFILE_50G_1_PAM4_RS544_COPPER,
         }},
        {1,
         {
             cfg::PortProfileID::PROFILE_50G_1_PAM4_RS544_COPPER,
         }},
        {2,
         {
             cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_COPPER,
             cfg::PortProfileID::PROFILE_50G_1_PAM4_RS544_COPPER,
         }},
        {3,
         {
             cfg::PortProfileID::PROFILE_50G_1_PAM4_RS544_COPPER,
         }},
};

static const std::unordered_map<int, std::vector<cfg::PortProfileID>>
    k8PortProfilesInGroup = {
        {0,
         {
             cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL,
             cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL,
             cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER,
             cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
        {1,
         {
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
        {2,
         {
             cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL,
             cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER,
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
        {3,
         {
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
        {4,
         {
             cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL,
             cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER,
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
        {5,
         {
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
        {6,
         {
             cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL,
             cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER,
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
        {7,
         {
             cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
             cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         }},
};

static const std::unordered_map<
    cfg::PortProfileID,
    std::tuple<
        cfg::PortSpeed,
        int,
        phy::FecMode,
        TransmitterTechnology,
        phy::InterfaceMode,
        phy::InterfaceType>>
    kProfiles = {
        {cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_OPTICAL,
         std::make_tuple(
             cfg::PortSpeed::HUNDREDG,
             2,
             phy::FecMode::RS544_2N,
             TransmitterTechnology::OPTICAL,
             phy::InterfaceMode::CAUI,
             phy::InterfaceType::CAUI)},
        {cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL,
         std::make_tuple(
             cfg::PortSpeed::HUNDREDG,
             4,
             phy::FecMode::CL91,
             TransmitterTechnology::OPTICAL,
             phy::InterfaceMode::CAUI,
             phy::InterfaceType::CAUI)},
        {cfg::PortProfileID::PROFILE_50G_2_NRZ_CL74_COPPER,
         std::make_tuple(
             cfg::PortSpeed::FIFTYG,
             2,
             phy::FecMode::CL74,
             TransmitterTechnology::COPPER,
             phy::InterfaceMode::CR2,
             phy::InterfaceType::CR2)},
        {cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC_OPTICAL,
         std::make_tuple(
             cfg::PortSpeed::FORTYG,
             4,
             phy::FecMode::NONE,
             TransmitterTechnology::OPTICAL,
             phy::InterfaceMode::XLAUI,
             phy::InterfaceType::XLAUI)},
        {cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL,
         std::make_tuple(
             cfg::PortSpeed::TWENTYFIVEG,
             1,
             phy::FecMode::NONE,
             TransmitterTechnology::OPTICAL,
             phy::InterfaceMode::CAUI,
             phy::InterfaceType::CAUI)},
        {cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL,
         std::make_tuple(
             cfg::PortSpeed::XG,
             1,
             phy::FecMode::NONE,
             TransmitterTechnology::OPTICAL,
             phy::InterfaceMode::SFI,
             phy::InterfaceType::SFI)},
        {cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_COPPER,
         std::make_tuple(
             cfg::PortSpeed::FOURHUNDREDG,
             8,
             phy::FecMode::RS544_2N,
             TransmitterTechnology::COPPER,
             phy::InterfaceMode::CAUI,
             phy::InterfaceType::CAUI)},
        {cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER,
         std::make_tuple(
             cfg::PortSpeed::TWOHUNDREDG,
             4,
             phy::FecMode::RS544_2N,
             TransmitterTechnology::COPPER,
             phy::InterfaceMode::CAUI,
             phy::InterfaceType::CAUI)},
        {cfg::PortProfileID::PROFILE_100G_2_PAM4_RS544X2N_COPPER,
         std::make_tuple(
             cfg::PortSpeed::HUNDREDG,
             2,
             phy::FecMode::RS544_2N,
             TransmitterTechnology::COPPER,
             phy::InterfaceMode::CAUI,
             phy::InterfaceType::CAUI)},
        {cfg::PortProfileID::PROFILE_50G_1_PAM4_RS544_COPPER,
         std::make_tuple(
             cfg::PortSpeed::FIFTYG,
             1,
             phy::FecMode::RS544_2N,
             TransmitterTechnology::COPPER,
             phy::InterfaceMode::CAUI,
             phy::InterfaceType::CAUI)},
};
} // namespace

namespace facebook::fboss {
FakeTestPlatformMapping::FakeTestPlatformMapping(
    std::vector<int> controllingPortIds,
    int portsPerSlot,
    bool twoTransceiversPerPort)
    : PlatformMapping(), controllingPortIds_(std::move(controllingPortIds)) {
  for (auto itProfile : kProfiles) {
    phy::PortProfileConfig profile;
    *profile.speed() = std::get<0>(itProfile.second);
    *profile.iphy()->numLanes() = std::get<1>(itProfile.second);
    *profile.iphy()->fec() = std::get<2>(itProfile.second);
    profile.iphy()->medium() = std::get<3>(itProfile.second);
    profile.iphy()->interfaceMode() = std::get<4>(itProfile.second);
    profile.iphy()->interfaceType() = std::get<5>(itProfile.second);

    phy::ProfileSideConfig xphySys;
    xphySys.numLanes() = std::get<1>(itProfile.second);
    xphySys.fec() = std::get<2>(itProfile.second);
    profile.xphySystem() = xphySys;

    phy::ProfileSideConfig xphyLine;
    xphyLine.numLanes() = std::get<1>(itProfile.second);
    xphyLine.fec() = std::get<2>(itProfile.second);
    profile.xphyLine() = xphyLine;

    cfg::PlatformPortProfileConfigEntry configEntry;
    cfg::PlatformPortConfigFactor factor;
    factor.profileID() = itProfile.first;
    configEntry.profile() = profile;
    configEntry.factor() = factor;
    mergePlatformSupportedProfile(configEntry);
  }

  if (twoTransceiversPerPort) {
    createDualTransceiverPlatformMapping(portsPerSlot);
  } else {
    createPlatformMapping(portsPerSlot);
  }

  // Each group has portsPerSlot ports regardless of configuration
  CHECK_EQ(
      getPlatformPorts().size(), controllingPortIds_.size() * portsPerSlot);
}

void FakeTestPlatformMapping::createPlatformMapping(int portsPerSlot) {
  for (int groupID = 0; groupID < controllingPortIds_.size(); groupID++) {
    auto portsInGroup = getPlatformPortEntriesByGroup(groupID, portsPerSlot);
    for (auto port : portsInGroup) {
      setPlatformPort(*port.mapping()->id(), port);
    }

    phy::DataPlanePhyChip iphy;
    *iphy.name() = folly::sformat("core{}", groupID);
    *iphy.type() = phy::DataPlanePhyChipType::IPHY;
    *iphy.physicalID() = groupID;
    setChip(*iphy.name(), iphy);

    phy::DataPlanePhyChip xphy;
    xphy.name() = folly::sformat("XPHY{}", groupID);
    xphy.type() = phy::DataPlanePhyChipType::XPHY;
    xphy.physicalID() = groupID;
    setChip(*xphy.name(), xphy);

    phy::DataPlanePhyChip tcvr;
    *tcvr.name() = folly::sformat("eth1/{}", groupID + 1);
    *tcvr.type() = phy::DataPlanePhyChipType::TRANSCEIVER;
    *tcvr.physicalID() = groupID;
    setChip(*tcvr.name(), tcvr);
  }
}

cfg::PlatformPortConfig FakeTestPlatformMapping::getPlatformPortConfig(
    int portID,
    int startLane,
    int groupID,
    cfg::PortProfileID profileID) {
  cfg::PlatformPortConfig platformPortConfig;
  auto& profileTuple = kProfiles.find(profileID)->second;
  auto lanes = std::get<1>(profileTuple);
  platformPortConfig.subsumedPorts() = {};
  for (auto i = 1; i < lanes; i++) {
    platformPortConfig.subsumedPorts()->push_back(portID + i);
  }

  platformPortConfig.pins()->transceiver() = {};
  for (auto i = 0; i < lanes; i++) {
    phy::PinConfig iphy;
    *iphy.id()->chip() = folly::sformat("core{}", groupID);
    *iphy.id()->lane() = (startLane + i);
    // tx config
    iphy.tx() = getFakeTxSetting();
    // rx configs
    iphy.rx() = getFakeRxSetting();
    platformPortConfig.pins()->iphy()->push_back(iphy);

    phy::PinConfig xphy;
    xphy.id()->chip() = folly::sformat("XPHY{}", groupID);
    xphy.id()->lane() = (startLane + i);
    xphy.tx() = getFakeTxSetting();
    xphy.rx() = getFakeRxSetting();
    platformPortConfig.pins()->xphySys() = {xphy};
    platformPortConfig.pins()->xphyLine() = {xphy};

    phy::PinConfig tcvr;
    *tcvr.id()->chip() = folly::sformat("eth1/{}", groupID + 1);
    *tcvr.id()->lane() = (startLane + i);
    platformPortConfig.pins()->transceiver()->push_back(tcvr);
  }

  return platformPortConfig;
}

std::vector<cfg::PlatformPortEntry>
FakeTestPlatformMapping::getPlatformPortEntriesByGroup(
    int groupID,
    int portsPerSlot,
    bool twoTransceiversPerPort) {
  std::unordered_map<int, std::vector<cfg::PortProfileID>> kPortProfilesInGroup;
  switch (portsPerSlot) {
    case 8:
      kPortProfilesInGroup = k8PortProfilesInGroup;
      break;
    case 1:
      kPortProfilesInGroup = k1PortProfilesInGroup;
      break;
    default:
      kPortProfilesInGroup = k4PortProfilesInGroup;
  }
  if (twoTransceiversPerPort) {
    kPortProfilesInGroup = k4PortProfilesInGroupForMultiTcvr;
  }

  std::vector<cfg::PlatformPortEntry> platformPortEntries;
  for (auto& portProfiles : kPortProfilesInGroup) {
    int portID = controllingPortIds_.at(groupID) + portProfiles.first;
    cfg::PlatformPortEntry port;
    *port.mapping()->id() = PortID(portID);
    *port.mapping()->name() =
        folly::sformat("eth1/{}/{}", groupID + 1, portProfiles.first + 1);
    *port.mapping()->controllingPort() = controllingPortIds_.at(groupID);

    phy::PinConnection asicPinConnection;
    asicPinConnection.a()->chip() = folly::sformat("core{}", groupID);
    asicPinConnection.a()->lane() = portProfiles.first;

    phy::PinConnection xphyLinePinConnection;
    xphyLinePinConnection.a()->chip() = folly::sformat("XPHY{}", groupID);
    xphyLinePinConnection.a()->lane() = portProfiles.first;

    phy::PinID pinEnd;
    *pinEnd.chip() = folly::sformat("eth1/{}", groupID + 1);
    *pinEnd.lane() = portProfiles.first;

    phy::Pin zPin;
    zPin.end() = pinEnd;
    xphyLinePinConnection.z() = zPin;

    phy::PinJunction xphyPinJunction;
    xphyPinJunction.system()->chip() = folly::sformat("XPHY{}", groupID);
    xphyPinJunction.system()->lane() = portProfiles.first;
    xphyPinJunction.line() = {xphyLinePinConnection};

    phy::Pin xphyPin;
    xphyPin.junction() = xphyPinJunction;

    asicPinConnection.z() = xphyPin;

    port.mapping()->pins()->push_back(asicPinConnection);

    for (auto profileID : portProfiles.second) {
      port.supportedProfiles()->emplace(
          profileID,
          getPlatformPortConfig(
              portID, portProfiles.first, groupID, profileID));
    }

    platformPortEntries.push_back(port);
  }
  return platformPortEntries;
}

phy::TxSettings FakeTestPlatformMapping::getFakeTxSetting() {
  phy::TxSettings tx;
  tx.pre() = 101;
  tx.pre2() = 102;
  tx.main() = 103;
  tx.post() = -104;
  tx.post2() = 105;
  tx.post3() = 106;
  tx.driveCurrent() = 107;
  return tx;
}

phy::RxSettings FakeTestPlatformMapping::getFakeRxSetting() {
  phy::RxSettings rx;
  rx.ctlCode() = -108;
  rx.dspMode() = 109;
  rx.afeTrim() = 110;
  rx.acCouplingBypass() = -111;
  return rx;
}

void FakeTestPlatformMapping::createDualTransceiverPlatformMapping(
    int portsPerSlot) {
  // PlatformMapping Structure
  // Two controlling ports - one TransceiverId assigned to each
  // First controlling port can subsume the lanes of the second transceiver, so
  // will have one extra profile to accommodate this otherwise each transceiver
  // is the same.

  // This helper function is tied to a distinct use case of rev Y-cable, so we
  // constrict the requirements on input args.
  if (controllingPortIds_.size() != 2 || portsPerSlot != 4) {
    throw FbossError(
        folly::sformat(
            "twoTransceiversPerPort is enabled, but controllingPortIds size is {} (expected 2) or portsPerSlot is {} (expected 4).",
            controllingPortIds_.size(),
            portsPerSlot));
  }

  auto addDualTransceiverProfile =
      [](cfg::PlatformPortEntry& firstPort, int portsPerSlot, int groupID) {
        // Custom logic for first port to subsume next port.
        // Changing Static Mapping
        auto firstPortId = *firstPort.mapping()->id();
        std::vector<phy::PinConnection> dualTcvrPinConnections;
        for (int i = 0; i < 4; i++) {
          phy::PinConnection asicPinConnection;
          asicPinConnection.a()->chip() = folly::sformat("core{}", groupID);
          asicPinConnection.a()->lane() = i;

          phy::PinID pinEnd1;
          *pinEnd1.chip() = folly::sformat("eth1/{}", groupID + 1);
          *pinEnd1.lane() = i;

          phy::Pin zPin1;
          zPin1.end() = pinEnd1;
          asicPinConnection.z() = zPin1;
          dualTcvrPinConnections.push_back(asicPinConnection);
        }

        // Second Tcvr Lanes
        for (int i = 0; i < 4; i++) {
          phy::PinConnection asicPinConnection;
          asicPinConnection.a()->chip() = folly::sformat("core{}", groupID);
          asicPinConnection.a()->lane() = i + 4;

          phy::PinID pinEnd1;
          *pinEnd1.chip() = folly::sformat("eth1/{}", groupID + 2);
          *pinEnd1.lane() = i;

          phy::Pin zPin1;
          zPin1.end() = pinEnd1;
          asicPinConnection.z() = zPin1;
          dualTcvrPinConnections.push_back(asicPinConnection);
        }

        firstPort.mapping()->pins() = dualTcvrPinConnections;

        // Adding new supported profile.
        std::vector<phy::PinConfig> dualTcvrPinConfigsPhy;
        for (int i = 0; i < 8; i++) {
          phy::PinConfig iphy;
          *iphy.id()->chip() = folly::sformat("core{}", groupID);
          *iphy.id()->lane() = i;
          iphy.tx() = getFakeTxSetting();
          iphy.rx() = getFakeRxSetting();
          dualTcvrPinConfigsPhy.push_back(iphy);
        }

        std::vector<phy::PinConfig> dualTcvrPinConfigsTcvr;
        for (int i = 0; i < 4; i++) {
          phy::PinConfig tcvr;
          *tcvr.id()->chip() = folly::sformat("eth1/{}", groupID + 1);
          *tcvr.id()->lane() = i;
          dualTcvrPinConfigsTcvr.push_back(tcvr);
        }
        for (int i = 0; i < 4; i++) {
          phy::PinConfig tcvr;
          *tcvr.id()->chip() = folly::sformat("eth1/{}", groupID + 2);
          *tcvr.id()->lane() = i;
          dualTcvrPinConfigsTcvr.push_back(tcvr);
        }

        // Dual Transceiver Port Profile (Using all 8 lanes to same ASIC core)
        cfg::PlatformPortConfig dualTcvrConfig;
        std::vector<int> subsumedPorts;
        for (int i = 1; i < portsPerSlot; i++) {
          subsumedPorts.push_back(firstPortId + i);
        }
        dualTcvrConfig.subsumedPorts() = subsumedPorts;
        dualTcvrConfig.pins()->iphy() = dualTcvrPinConfigsPhy;
        dualTcvrConfig.pins()->transceiver() = dualTcvrPinConfigsTcvr;

        firstPort.supportedProfiles()->emplace(
            cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_COPPER,
            dualTcvrConfig);
      };

  for (int groupID = 0; groupID < controllingPortIds_.size(); groupID++) {
    auto portsInGroup =
        getPlatformPortEntriesByGroup(groupID, portsPerSlot, true);

    for (auto& port : portsInGroup) {
      if (groupID == 0 && *port.mapping()->id() == 1) {
        addDualTransceiverProfile(port, portsPerSlot, groupID);
      }
      setPlatformPort(*port.mapping()->id(), port);
    }

    phy::DataPlanePhyChip iphy;
    *iphy.name() = folly::sformat("core{}", groupID);
    *iphy.type() = phy::DataPlanePhyChipType::IPHY;
    *iphy.physicalID() = groupID;
    setChip(*iphy.name(), iphy);

    phy::DataPlanePhyChip xphy;
    xphy.name() = folly::sformat("XPHY{}", groupID);
    xphy.type() = phy::DataPlanePhyChipType::XPHY;
    xphy.physicalID() = groupID;
    setChip(*xphy.name(), xphy);

    phy::DataPlanePhyChip tcvr;
    *tcvr.name() = folly::sformat("eth1/{}", groupID + 1);
    *tcvr.type() = phy::DataPlanePhyChipType::TRANSCEIVER;
    *tcvr.physicalID() = groupID;
    setChip(*tcvr.name(), tcvr);
  }
}

} // namespace facebook::fboss
