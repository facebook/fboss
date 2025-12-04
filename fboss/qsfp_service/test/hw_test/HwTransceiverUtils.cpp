/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/HwTransceiverUtils.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/lib/config/PlatformConfigUtils.h"
#include "fboss/qsfp_service/TransceiverManager.h"

namespace facebook::fboss::utility {

void HwTransceiverUtils::verifyTempAndVccFlags(
    std::map<std::string, TransceiverInfo>& portToTransceiverInfoMap) {
  for (auto& [_, transceiverInfo] : portToTransceiverInfoMap) {
    auto tcvrID = *transceiverInfo.tcvrState()->port();
    auto& tcvrStats = *transceiverInfo.tcvrStats();
    EXPECT_FALSE(
        *tcvrStats.sensor()->temp()->flags().value_or({}).alarm()->high())
        << folly::sformat("{:d} has high temp alarm flag", tcvrID);
    EXPECT_FALSE(
        *tcvrStats.sensor()->temp()->flags().value_or({}).warn()->high())
        << folly::sformat("{:d} has high temp warn flag", tcvrID);
    EXPECT_FALSE(
        *tcvrStats.sensor()->vcc()->flags().value_or({}).alarm()->high())
        << folly::sformat("{:d} has high vcc alarm flag", tcvrID);
    EXPECT_FALSE(
        *tcvrStats.sensor()->vcc()->flags().value_or({}).warn()->high())
        << folly::sformat("{:d} has high vcc warn flag", tcvrID);
  }
}

void HwTransceiverUtils::verifyPortNameToLaneMap(
    const std::vector<PortID>& portIDs,
    cfg::PortProfileID profile,
    const PlatformMapping* platformMapping,
    std::map<int32_t, TransceiverInfo>& tcvrInfos) {
  if (profile == cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER ||
      profile == cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL) {
    // We use these profiles on Meru400biu and Meru400bfu with 200G optics in
    // a hacky configuration which invalidates the verification of media/host
    // lanes in this function.
    return;
  }
  const auto& platformPorts = platformMapping->getPlatformPorts();
  const auto& chips = platformMapping->getChips();
  for (auto portID : portIDs) {
    auto hostLanesFromPlatformMapping =
        platformMapping->getTransceiverHostLanes(
            PlatformPortProfileConfigMatcher(
                profile, portID, std::nullopt /* portConfigOverrideFactor */));
    EXPECT_NE(hostLanesFromPlatformMapping.size(), 0);
    auto platformPortItr = platformPorts.find(static_cast<int32_t>(portID));
    ASSERT_NE(platformPortItr, platformPorts.end());
    auto tcvrIds = utility::getTransceiverIds(platformPortItr->second, chips);
    ASSERT_EQ(tcvrIds.size(), 1);
    auto portName = *platformPortItr->second.mapping()->name();

    auto tcvrInfoItr = tcvrInfos.find(tcvrIds[0]);
    ASSERT_NE(tcvrInfoItr, tcvrInfos.end());

    auto& hostLaneMap = *tcvrInfoItr->second.tcvrState()->portNameToHostLanes();
    // Verify port exists in the map
    EXPECT_NE(hostLaneMap.find(portName), hostLaneMap.end());
    // Verify we have the same host lanes in the map as returned by platform
    // mapping
    EXPECT_EQ(
        hostLaneMap[portName].size(), hostLanesFromPlatformMapping.size());
    for (auto lane : hostLanesFromPlatformMapping) {
      XLOG(INFO) << "Verifying lane " << lane << " on " << portName;
      EXPECT_NE(
          std::find(
              hostLaneMap[portName].begin(), hostLaneMap[portName].end(), lane),
          hostLaneMap[portName].end());
    }

    auto& mediaLaneMap =
        *tcvrInfoItr->second.tcvrState()->portNameToMediaLanes();
    std::vector<int> expectedMediaLanes;
    auto moduleMediaInterface =
        tcvrInfoItr->second.tcvrState()->moduleMediaInterface().ensure();

    // Note for multi-port transceivers the below switch case may also need to
    // look at speed + port + platform type. Will modify it once we get the
    // multi-port transceivers
    switch (moduleMediaInterface) {
      case MediaInterfaceCode::CWDM4_100G:
        if (profile == cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_OPTICAL) {
          expectedMediaLanes = {0, 1};
        } else if (
            profile == cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL) {
          auto hostLanes = hostLaneMap[portName];
          ASSERT_TRUE(
              hostLanes == std::vector{0} || hostLanes == std::vector{1});
          expectedMediaLanes = hostLanes;
        } else {
          expectedMediaLanes = {0, 1, 2, 3};
        }
        break;
      case MediaInterfaceCode::FR4_200G:
      case MediaInterfaceCode::LR4_200G:
      case MediaInterfaceCode::FR4_400G:
      case MediaInterfaceCode::DR4_400G:
      case MediaInterfaceCode::LR4_400G_10KM:
        expectedMediaLanes = {0, 1, 2, 3};
        break;
      case MediaInterfaceCode::FR4_2x400G:
      case MediaInterfaceCode::FR4_LITE_2x400G:
      case MediaInterfaceCode::FR4_LPO_2x400G:
      case MediaInterfaceCode::LR4_2x400G_10KM:
      case MediaInterfaceCode::DR4_2x400G:
      case MediaInterfaceCode::DR4_2x800G:
        switch (profile) {
          case cfg::PortProfileID::PROFILE_800G_4_PAM4_RS544X2N_OPTICAL:
          case cfg::PortProfileID::PROFILE_400G_4_PAM4_RS544X2N_OPTICAL:
          case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL:
          case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL:
            if (std::find(
                    hostLaneMap[portName].begin(),
                    hostLaneMap[portName].end(),
                    0) != hostLaneMap[portName].end()) {
              // When lane 0 is one of the host lanes, the media lanes are
              // expected to be 0,1,2,3.
              expectedMediaLanes = {0, 1, 2, 3};
            } else {
              expectedMediaLanes = {4, 5, 6, 7};
            }
            break;
          case cfg::PortProfileID::PROFILE_106POINT25G_1_PAM4_RS544_OPTICAL:
          case cfg::PortProfileID::PROFILE_100G_1_PAM4_RS544_OPTICAL:
          case cfg::PortProfileID::PROFILE_200G_1_PAM4_RS544X2N_OPTICAL:
            expectedMediaLanes = {*hostLaneMap[portName].begin()};
            break;
          case cfg::PortProfileID::PROFILE_800G_8_PAM4_RS544X2N_OPTICAL:
            expectedMediaLanes = {0, 1, 2, 3, 4, 5, 6, 7};
            break;
          case cfg::PortProfileID::PROFILE_400G_2_PAM4_RS544X2N_OPTICAL:
            if (std::find(
                    hostLaneMap[portName].begin(),
                    hostLaneMap[portName].end(),
                    0) != hostLaneMap[portName].end()) {
              // When lane 0 is one of the host lanes, the media lanes are
              // expected to be 0,1
              expectedMediaLanes = {0, 1};
            } else if (
                std::find(
                    hostLaneMap[portName].begin(),
                    hostLaneMap[portName].end(),
                    2) != hostLaneMap[portName].end()) {
              // When lane 2 is one of the host lanes, the media lanes are
              // expected to be 2,3
              expectedMediaLanes = {2, 3};
            } else if (
                std::find(
                    hostLaneMap[portName].begin(),
                    hostLaneMap[portName].end(),
                    4) != hostLaneMap[portName].end()) {
              // When lane 4 is one of the host lanes, the media lanes are
              // expected to be 4,5
              expectedMediaLanes = {4, 5};
            } else {
              expectedMediaLanes = {6, 7};
            }
            break;
          default:
            throw FbossError(
                "Unhandled profile ",
                apache::thrift::util::enumNameSafe(profile));
        }
        break;
      case MediaInterfaceCode::CR8_800G:
        switch (profile) {
          case cfg::PortProfileID::PROFILE_100G_1_PAM4_RS544X2N_COPPER: {
            auto itr = hostLaneMap.find(portName);
            CHECK(itr != hostLaneMap.end());
            auto& hostLaneVector = itr->second;
            CHECK_EQ(hostLaneVector.size(), 1);
            expectedMediaLanes = hostLaneVector;
          } break;
          case cfg::PortProfileID::PROFILE_400G_4_PAM4_RS544X2N_COPPER:
            if (std::find(
                    hostLaneMap[portName].begin(),
                    hostLaneMap[portName].end(),
                    0) != hostLaneMap[portName].end()) {
              // When lane 0 is one of the host lanes, the media lanes are
              // expected to be 0,1,2,3.
              expectedMediaLanes = {0, 1, 2, 3};
            } else {
              expectedMediaLanes = {4, 5, 6, 7};
            }
            break;
          case cfg::PortProfileID::PROFILE_800G_8_PAM4_RS544X2N_COPPER:
            expectedMediaLanes = {0, 1, 2, 3, 4, 5, 6, 7};
            break;
          default:
            throw FbossError(
                "Unhandled profile ",
                apache::thrift::util::enumNameSafe(profile));
        }
        break;
      case MediaInterfaceCode::FR1_100G:
      case MediaInterfaceCode::LR_10G:
      case MediaInterfaceCode::SR_10G:
      case MediaInterfaceCode::BASE_T_10G:
      case MediaInterfaceCode::CR_10G:
      case MediaInterfaceCode::ZR_800G:
      case MediaInterfaceCode::CR1_100G:
        expectedMediaLanes = {0};
        break;
      case MediaInterfaceCode::UNKNOWN:
      case MediaInterfaceCode::CR4_100G:
      case MediaInterfaceCode::CR4_200G:
      case MediaInterfaceCode::CR4_400G:
      case MediaInterfaceCode::CR8_400G:
        expectedMediaLanes = {};
        break;
      case MediaInterfaceCode::FR8_800G:
        expectedMediaLanes = {0, 1, 2, 3, 4, 5, 6, 7};
        break;
      case MediaInterfaceCode::DR2_400G:
      case MediaInterfaceCode::DR4_800G:
      case MediaInterfaceCode::DR1_200G:
      case MediaInterfaceCode::DR1_100G: {
        throw FbossError(
            "Unsupported moduleMediaInterface ",
            apache::thrift::util::enumNameSafe(moduleMediaInterface));
      }
    }

    XLOG(INFO) << "Verifying that " << portName << " uses media lanes "
               << folly::join(",", expectedMediaLanes);
    // Verify port exists in the map
    EXPECT_NE(mediaLaneMap.find(portName), mediaLaneMap.end());
    EXPECT_EQ(mediaLaneMap.at(portName), expectedMediaLanes);

    // Expect both tcvrState and tcvrStats to have the same map
    EXPECT_EQ(
        *tcvrInfoItr->second.tcvrState()->portNameToHostLanes(),
        *tcvrInfoItr->second.tcvrStats()->portNameToHostLanes());
    EXPECT_EQ(
        *tcvrInfoItr->second.tcvrState()->portNameToMediaLanes(),
        *tcvrInfoItr->second.tcvrStats()->portNameToMediaLanes());
  }
}

void HwTransceiverUtils::verifyTransceiverSettings(
    const TcvrState& tcvrState,
    const std::string& portName,
    cfg::PortProfileID profile) {
  auto id = *tcvrState.port();
  if (!*tcvrState.present()) {
    XLOG(INFO) << " Skip verifying: " << id << ", not present";
    return;
  }

  XLOG(INFO) << " Verifying: " << id << ", portName = " << portName;
  // Only testing QSFP and SFP transceivers right now
  EXPECT_TRUE(
      *tcvrState.transceiver() == TransceiverType::QSFP ||
      *tcvrState.transceiver() == TransceiverType::SFP);

  if (!TransceiverManager::opticalOrActiveCable(tcvrState)) {
    XLOG(INFO) << " Skip verifying tcvr settings: " << *tcvrState.port()
               << ", for passive copper cable";
  } else if (
      profile != cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER &&
      profile != cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL) {
    // We use these profiles on Meru400biu and Meru400bfu with 200G optics in
    // a hacky configuration which invalidates the verification of optics
    // settings in this function.
    verifyOpticsSettings(tcvrState, portName, profile);
  }

  verifyMediaInterfaceCompliance(tcvrState, profile, portName);

  if (profile != cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER &&
      profile != cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL) {
    // We use these profiles on Meru400biu and Meru400bfu with 200G optics in
    // a hacky configuration which invalidates the verification of datapath in
    // this function.
    verifyDataPathEnabled(tcvrState, portName);
  }
}

void HwTransceiverUtils::verifyOpticsSettings(
    const TcvrState& tcvrState,
    const std::string& portName,
    cfg::PortProfileID profile) {
  auto settings = apache::thrift::can_throw(*tcvrState.settings());
  const auto& portToMediaMap = *tcvrState.portNameToMediaLanes();
  const auto& portToHostMap = *tcvrState.portNameToHostLanes();

  CHECK(portToMediaMap.find(portName) != portToMediaMap.end());
  CHECK(portToHostMap.find(portName) != portToHostMap.end());

  const auto& relevantMediaLanes = portToMediaMap.at(portName);
  const auto& relevantHostLanes = portToHostMap.at(portName);

  EXPECT_GT(relevantMediaLanes.size(), 0);
  EXPECT_GT(relevantHostLanes.size(), 0);

  for (auto& mediaLane :
       apache::thrift::can_throw(*settings.mediaLaneSettings())) {
    if (std::find(
            relevantMediaLanes.begin(),
            relevantMediaLanes.end(),
            *mediaLane.lane()) == relevantMediaLanes.end()) {
      continue;
    }
    EXPECT_FALSE(*mediaLane.txDisable())
        << "Transceiver:" << *tcvrState.port() << ", Lane=" << *mediaLane.lane()
        << " txDisable doesn't match expected";
    if (*tcvrState.transceiver() == TransceiverType::QSFP) {
      EXPECT_EQ(
          *mediaLane.txSquelch(),
          profile ==
              cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL)
          << "Transceiver:" << *tcvrState.port()
          << ", Lane=" << *mediaLane.lane()
          << " txSquelch doesn't match expected." << " Profile was "
          << apache::thrift::util::enumNameSafe(profile);
    }
  }

  if (*tcvrState.transceiver() == TransceiverType::QSFP) {
    // Disable low power mode
    EXPECT_TRUE(
        *settings.powerControl() == PowerControlState::POWER_OVERRIDE ||
        *settings.powerControl() == PowerControlState::HIGH_POWER_OVERRIDE);

    // TODO: T236126124 - Disable checking this in Molex AEC cables
    // until we get EEPROM fix.
    auto vendor = apache::thrift::can_throw(*tcvrState.vendor());
    bool isMolex =
        (vendor.name() == "Molex" && vendor.partNumber() == "2253611207");

    // LPO Modules dont have a DSP, so we dont need to check for CDR.
    bool isLpoModule = tcvrState.lpoModule().value();
    if (!(isMolex || isLpoModule)) {
      EXPECT_EQ(*settings.cdrTx(), FeatureState::ENABLED);
      EXPECT_EQ(*settings.cdrRx(), FeatureState::ENABLED);
    }

    for (auto& hostLane :
         apache::thrift::can_throw(*settings.hostLaneSettings())) {
      if (std::find(
              relevantHostLanes.begin(),
              relevantHostLanes.end(),
              *hostLane.lane()) == relevantHostLanes.end()) {
        continue;
      }
      EXPECT_EQ(
          *hostLane.rxSquelch(),
          profile ==
              cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL)
          << "Transceiver:" << *tcvrState.port()
          << ", Lane=" << *hostLane.lane()
          << " rxSquelch doesn't match expected";
    }
  }
}

void HwTransceiverUtils::verifyMediaInterfaceCompliance(
    const TcvrState& tcvrState,
    cfg::PortProfileID profile,
    const std::string& portName) {
  auto settings = apache::thrift::can_throw(*tcvrState.settings());
  auto mgmtInterface =
      apache::thrift::can_throw(*tcvrState.transceiverManagementInterface());
  EXPECT_TRUE(
      mgmtInterface == TransceiverManagementInterface::SFF8472 ||
      mgmtInterface == TransceiverManagementInterface::SFF ||
      mgmtInterface == TransceiverManagementInterface::CMIS);
  auto allMediaInterfaces =
      apache::thrift::can_throw(*settings.mediaInterface());
  std::vector<MediaInterfaceId> mediaInterfaces;
  // Filter out the mediaInterfaces for this specific port
  for (auto mediaLane : tcvrState.portNameToMediaLanes()->at(portName)) {
    ASSERT_TRUE(mediaLane < allMediaInterfaces.size());
    mediaInterfaces.push_back(allMediaInterfaces[mediaLane]);
  }

  switch (profile) {
    case cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL:
      verify10gProfile(tcvrState, mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_25G_1_NRZ_NOFEC_OPTICAL:
      verify25gProfile(mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_50G_2_NRZ_RS528_OPTICAL:
      verify50gProfile(mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL:
    case cfg::PortProfileID::PROFILE_100G_1_PAM4_RS544_OPTICAL:
    case cfg::PortProfileID::PROFILE_106POINT25G_1_PAM4_RS544_OPTICAL:
      verify100gProfile(mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC:
      // TODO
      break;

    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N:
    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL:
    case cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL:
    case cfg::PortProfileID::PROFILE_200G_1_PAM4_RS544X2N_OPTICAL:
      verify200gProfile(mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL:
    case cfg::PortProfileID::PROFILE_400G_4_PAM4_RS544X2N_OPTICAL:
    case cfg::PortProfileID::PROFILE_400G_2_PAM4_RS544X2N_OPTICAL:
      verify400gProfile(mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_COPPER:
    case cfg::PortProfileID::PROFILE_400G_4_PAM4_RS544X2N_COPPER:
      verifyCopper400gProfile(tcvrState, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_100G_1_PAM4_RS544X2N_COPPER:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER:
      verifyCopper100gProfile(tcvrState, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER:
      verifyCopper200gProfile(tcvrState, mediaInterfaces);
      break;
    case cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER:
      verifyCopper53gProfile(tcvrState, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_800G_4_PAM4_RS544X2N_OPTICAL:
    case cfg::PortProfileID::PROFILE_800G_8_PAM4_RS544X2N_OPTICAL:
      verifyOptical800gProfile(mgmtInterface, mediaInterfaces);
      break;
    case cfg::PortProfileID::PROFILE_800G_8_PAM4_RS544X2N_COPPER:
      verifyCopper800gProfile(tcvrState, mediaInterfaces);
      break;
    default:
      throw FbossError(
          "Unhandled profile ", apache::thrift::util::enumNameSafe(profile));
  }
}

void HwTransceiverUtils::verify10gProfile(
    const TcvrState& tcvrState,
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::SFF8472);
  EXPECT_EQ(*tcvrState.transceiver(), TransceiverType::SFP);

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(
        *mediaId.media()->ethernet10GComplianceCode() ==
            Ethernet10GComplianceCode::LR_10G ||
        *mediaId.media()->ethernet10GComplianceCode() ==
            Ethernet10GComplianceCode::CR_10G);

    EXPECT_TRUE(
        *mediaId.code() == MediaInterfaceCode::LR_10G ||
        *mediaId.code() == MediaInterfaceCode::CR_10G);
  }
}

void HwTransceiverUtils::verify25gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::SFF);
    auto specComplianceCode =
        *mediaId.media()->extendedSpecificationComplianceCode();
    EXPECT_EQ(specComplianceCode, ExtendedSpecComplianceCode::CWDM4_100G);
  }
}

void HwTransceiverUtils::verify50gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::SFF);
    auto specComplianceCode =
        *mediaId.media()->extendedSpecificationComplianceCode();
    EXPECT_EQ(specComplianceCode, ExtendedSpecComplianceCode::CWDM4_100G);
  }
}

void HwTransceiverUtils::verify100gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  for (const auto& mediaId : mediaInterfaces) {
    if (mgmtInterface == TransceiverManagementInterface::SFF) {
      auto specComplianceCode =
          *mediaId.media()->extendedSpecificationComplianceCode();
      EXPECT_TRUE(
          specComplianceCode == ExtendedSpecComplianceCode::CWDM4_100G ||
          specComplianceCode == ExtendedSpecComplianceCode::FR1_100G ||
          specComplianceCode == ExtendedSpecComplianceCode::CR4_100G);
      EXPECT_TRUE(
          mediaId.code() == MediaInterfaceCode::CWDM4_100G ||
          mediaId.code() == MediaInterfaceCode::FR1_100G ||
          mediaId.code() == MediaInterfaceCode::CR4_100G);
    } else if (mgmtInterface == TransceiverManagementInterface::CMIS) {
      EXPECT_TRUE(
          *mediaId.media()->smfCode() == SMFMediaInterfaceCode::CWDM4_100G ||
          *mediaId.media()->smfCode() == SMFMediaInterfaceCode::FR1_100G ||
          *mediaId.media()->smfCode() == SMFMediaInterfaceCode::DR1_100G);
      EXPECT_TRUE(
          *mediaId.code() == MediaInterfaceCode::CWDM4_100G ||
          *mediaId.code() == MediaInterfaceCode::FR1_100G ||
          *mediaId.code() == MediaInterfaceCode::DR1_100G);
    }
  }
}

void HwTransceiverUtils::verify200gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::CMIS);

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(
        *mediaId.media()->smfCode() == SMFMediaInterfaceCode::FR4_200G ||
        *mediaId.media()->smfCode() == SMFMediaInterfaceCode::LR4_200G ||
        *mediaId.media()->smfCode() == SMFMediaInterfaceCode::DR1_200G);
    EXPECT_TRUE(
        *mediaId.code() == MediaInterfaceCode::FR4_200G ||
        *mediaId.code() == MediaInterfaceCode::LR4_200G ||
        *mediaId.code() == MediaInterfaceCode::DR1_200G);
  }
}

void HwTransceiverUtils::verify400gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::CMIS);

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(
        *mediaId.media()->smfCode() == SMFMediaInterfaceCode::FR4_400G ||
        *mediaId.media()->smfCode() == SMFMediaInterfaceCode::LR4_10_400G ||
        *mediaId.media()->smfCode() == SMFMediaInterfaceCode::DR4_400G ||
        *mediaId.media()->smfCode() == SMFMediaInterfaceCode::DR2_400G);
    EXPECT_TRUE(
        *mediaId.code() == MediaInterfaceCode::FR4_400G ||
        *mediaId.code() == MediaInterfaceCode::LR4_400G_10KM ||
        *mediaId.code() == MediaInterfaceCode::DR4_400G ||
        *mediaId.code() == MediaInterfaceCode::DR2_400G);
  }
}

void HwTransceiverUtils::verifyCopper400gProfile(
    const TcvrState& tcvrState,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  const auto& cable = *tcvrState.cable();
  const auto& transmitterTech = *cable.transmitterTech();
  const bool isActive = TransceiverManager::activeCable(tcvrState);
  auto mgmtInterface =
      apache::thrift::can_throw(*tcvrState.transceiverManagementInterface());

  EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::CMIS);
  EXPECT_EQ(TransmitterTechnology::COPPER, transmitterTech);

  for (const auto& mediaId : mediaInterfaces) {
    if (isActive) {
      EXPECT_TRUE(*mediaId.code() == MediaInterfaceCode::CR4_400G);
      EXPECT_TRUE(
          *mediaId.media()->activeCuCode() ==
          ActiveCuHostInterfaceCode::AUI_PAM4_4S_400G);
    } else {
      EXPECT_TRUE(*mediaId.code() == MediaInterfaceCode::CR8_400G);
    }
  }
}

void HwTransceiverUtils::verifyCopper100gProfile(
    const TcvrState& tcvrState,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(
      TransmitterTechnology::COPPER,
      *(tcvrState.cable().value_or({}).transmitterTech()));

  const bool isActive = TransceiverManager::activeCable(tcvrState);
  if (!isActive) {
  }
  for (const auto& mediaId : mediaInterfaces) {
    if (isActive) {
      EXPECT_TRUE(
          *mediaId.media()->activeCuCode() ==
          ActiveCuHostInterfaceCode::AUI_PAM4_1S_100G);
    } else {
      EXPECT_TRUE(*mediaId.code() == MediaInterfaceCode::CR4_100G);
    }
  }
}

void HwTransceiverUtils::verifyCopper200gProfile(
    const TcvrState& tcvrState,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(
      TransmitterTechnology::COPPER,
      *(tcvrState.cable().value_or({}).transmitterTech()));

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(*mediaId.code() == MediaInterfaceCode::CR4_200G);
  }
}

void HwTransceiverUtils::verifyCopper53gProfile(
    const TcvrState& tcvrState,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(
      TransmitterTechnology::COPPER,
      *(tcvrState.cable().value_or({}).transmitterTech()));

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(
        *mediaId.code() == MediaInterfaceCode::CR4_200G ||
        *mediaId.code() == MediaInterfaceCode::CR8_400G);
  }
}

void HwTransceiverUtils::verifyOptical800gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::CMIS);
  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(
        *mediaId.media()->smfCode() == SMFMediaInterfaceCode::FR8_800G ||
        *mediaId.media()->smfCode() == SMFMediaInterfaceCode::DR4_800G ||
        *mediaId.media()->smfCode() ==
            SMFMediaInterfaceCode::ZR_OROADM_FLEXO_8E_DPO_800G ||
        *mediaId.media()->smfCode() == SMFMediaInterfaceCode::ZR_OIF_ZRA_800G);
    EXPECT_TRUE(
        *mediaId.code() == MediaInterfaceCode::FR8_800G ||
        *mediaId.code() == MediaInterfaceCode::DR4_800G ||
        *mediaId.code() == MediaInterfaceCode::ZR_800G);
  }
}

void HwTransceiverUtils::verifyCopper800gProfile(
    const TcvrState& tcvrState,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  const auto& cable = *tcvrState.cable();
  const auto& transmitterTech = *cable.transmitterTech();
  const bool isActive = TransceiverManager::activeCable(tcvrState);
  auto mgmtInterface =
      apache::thrift::can_throw(*tcvrState.transceiverManagementInterface());

  EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::CMIS);
  EXPECT_EQ(TransmitterTechnology::COPPER, transmitterTech);

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(*mediaId.code() == MediaInterfaceCode::CR8_800G);
    if (isActive) {
      EXPECT_TRUE(
          *mediaId.media()->activeCuCode() ==
          ActiveCuHostInterfaceCode::AUI_PAM4_8S_800G);
    }
  }
}

void HwTransceiverUtils::verifyDataPathEnabled(
    const TcvrState& tcvrState,
    const std::string& portName) {
  auto mgmtInterface = tcvrState.transceiverManagementInterface();

  const auto& portToHostMap = *tcvrState.portNameToHostLanes();
  CHECK(portToHostMap.find(portName) != portToHostMap.end());
  const auto& relevantHostLanes = portToHostMap.at(portName);
  EXPECT_GT(relevantHostLanes.size(), 0);

  if (!mgmtInterface) {
    throw FbossError(
        "Transceiver:",
        *tcvrState.port(),
        " is missing transceiverManagementInterface");
  }
  // Right now, we only reset data path for CMIS module
  if (*mgmtInterface == TransceiverManagementInterface::CMIS) {
    // All lanes should have `false` Deinit
    if (auto hostLaneSignals = tcvrState.hostLaneSignals()) {
      for (const auto& laneSignals : *hostLaneSignals) {
        if (std::find(
                relevantHostLanes.begin(),
                relevantHostLanes.end(),
                *laneSignals.lane()) == relevantHostLanes.end()) {
          continue;
        }
        EXPECT_TRUE(laneSignals.dataPathDeInit().has_value())
            << "Transceiver:" << *tcvrState.port()
            << ", Lane=" << *laneSignals.lane()
            << " is missing dataPathDeInit signal";
        EXPECT_FALSE(*laneSignals.dataPathDeInit())
            << "Transceiver:" << *tcvrState.port()
            << ", Lane=" << *laneSignals.lane()
            << " dataPathDeInit doesn't match expected";
      }
    } else {
      throw FbossError(
          "Transceiver:", *tcvrState.port(), " is missing hostLaneSignals");
    }
  }
}

void HwTransceiverUtils::verifyDiagsCapability(
    const TcvrState& tcvrState,
    std::optional<DiagsCapability> diagsCapability,
    bool skipCheckingIndividualCapability) {
  auto mgmtInterface = tcvrState.transceiverManagementInterface();
  if (!mgmtInterface) {
    throw FbossError(
        "Transceiver:",
        *tcvrState.port(),
        " is missing transceiverManagementInterface");
  }
  auto mediaIntfCode = tcvrState.moduleMediaInterface();
  if (!mediaIntfCode) {
    throw FbossError(
        "Transceiver:", *tcvrState.port(), " is missing moduleMediaInterface");
  }
  XLOG(INFO) << "Verifying DiagsCapability for Transceiver="
             << *tcvrState.port() << ", mgmtInterface="
             << apache::thrift::util::enumNameSafe(*mgmtInterface)
             << ", moduleMediaInterface="
             << apache::thrift::util::enumNameSafe(*mediaIntfCode);

  switch (*mgmtInterface) {
    case TransceiverManagementInterface::CMIS: {
      if (!TransceiverManager::opticalOrActiveCable(tcvrState)) {
        // FlatMem modules don't support diagsCapability
        return;
      }
      EXPECT_TRUE(diagsCapability.has_value());
      if (!skipCheckingIndividualCapability) {
        EXPECT_TRUE(*diagsCapability->diagnostics());
        // Only 400G CMIS supports VDM (excluding AEC cables)
        EXPECT_EQ(
            *diagsCapability->vdm(),
            (*mediaIntfCode == MediaInterfaceCode::FR4_400G ||
             *mediaIntfCode == MediaInterfaceCode::LR4_400G_10KM ||
             *mediaIntfCode == MediaInterfaceCode::FR4_2x400G ||
             *mediaIntfCode == MediaInterfaceCode::FR4_LITE_2x400G ||
             *mediaIntfCode == MediaInterfaceCode::LR4_2x400G_10KM ||
             *mediaIntfCode == MediaInterfaceCode::DR4_2x400G ||
             *mediaIntfCode == MediaInterfaceCode::DR4_2x800G ||
             *mediaIntfCode == MediaInterfaceCode::ZR_800G));
        EXPECT_TRUE(*diagsCapability->cdb());
        EXPECT_TRUE(*diagsCapability->prbsLine());
        EXPECT_TRUE(*diagsCapability->prbsSystem());
        EXPECT_TRUE(*diagsCapability->loopbackLine());
        EXPECT_TRUE(*diagsCapability->loopbackSystem());
        EXPECT_TRUE(*diagsCapability->txOutputControl());
        if (*mediaIntfCode == MediaInterfaceCode::FR4_400G ||
            *mediaIntfCode == MediaInterfaceCode::LR4_400G_10KM ||
            *mediaIntfCode == MediaInterfaceCode::FR4_2x400G ||
            *mediaIntfCode == MediaInterfaceCode::FR4_LITE_2x400G ||
            *mediaIntfCode == MediaInterfaceCode::LR4_2x400G_10KM ||
            *mediaIntfCode == MediaInterfaceCode::DR4_2x400G ||
            *mediaIntfCode == MediaInterfaceCode::DR4_2x800G ||
            *mediaIntfCode == MediaInterfaceCode::ZR_800G ||
            *mediaIntfCode == MediaInterfaceCode::CR8_800G) {
          EXPECT_TRUE(*diagsCapability->rxOutputControl());
        }
      }
      return;
    }
    case TransceiverManagementInterface::SFF:
      // Only FR1_100G has diagsCapability
      if (*mediaIntfCode == MediaInterfaceCode::FR1_100G) {
        EXPECT_TRUE(diagsCapability.has_value());
        if (!skipCheckingIndividualCapability) {
          EXPECT_TRUE(*diagsCapability->prbsLine());
          EXPECT_TRUE(*diagsCapability->prbsSystem());
          EXPECT_TRUE(*diagsCapability->txOutputControl());
        }
      }
      return;
    case TransceiverManagementInterface::SFF8472:
    case TransceiverManagementInterface::NONE:
    case TransceiverManagementInterface::UNKNOWN:
      EXPECT_FALSE(diagsCapability.has_value());
      return;
  }
  throw FbossError(
      "Transceiver:",
      *tcvrState.port(),
      " is using unrecognized transceiverManagementInterface:",
      apache::thrift::util::enumNameSafe(*mgmtInterface));
}

void HwTransceiverUtils::verifyDatapathResetTimestamp(
    const std::string& portName,
    const TcvrState& tcvrState,
    const TcvrStats& tcvrStats,
    time_t timeReference,
    bool expectedReset) {
  if (TransceiverManager::opticalOrActiveCmisCable(tcvrState)) {
    auto& datapathResetTimestamp = *tcvrStats.lastDatapathResetTime();
    if (expectedReset) {
      ASSERT_TRUE(
          datapathResetTimestamp.find(portName) !=
          datapathResetTimestamp.end());
      EXPECT_GT(datapathResetTimestamp.at(portName), timeReference);
    } else {
      if (datapathResetTimestamp.find(portName) !=
          datapathResetTimestamp.end()) {
        EXPECT_LE(datapathResetTimestamp.at(portName), timeReference);
      }
    }
  }
}

} // namespace facebook::fboss::utility
