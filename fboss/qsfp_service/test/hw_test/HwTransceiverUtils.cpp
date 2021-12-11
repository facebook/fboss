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

#include "fboss/agent/FbossError.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss::utility {

void HwTransceiverUtils::verifyTransceiverSettings(
    const TransceiverInfo& transceiver,
    cfg::PortProfileID profile) {
  auto id = *transceiver.port_ref();
  if (!*transceiver.present_ref()) {
    XLOG(INFO) << " Skip verifying: " << id << ", not present";
    return;
  }

  XLOG(INFO) << " Verifying: " << id;
  // Only testing QSFP and SFP transceivers right now
  EXPECT_TRUE(
      *transceiver.transceiver_ref() == TransceiverType::QSFP ||
      *transceiver.transceiver_ref() == TransceiverType::SFP);

  if (TransmitterTechnology::COPPER ==
      *(transceiver.cable_ref().value_or({}).transmitterTech_ref())) {
    XLOG(INFO) << " Skip verifying optics settings: " << *transceiver.port_ref()
               << ", for copper cable";
  } else {
    verifyOpticsSettings(transceiver);
  }

  verifyMediaInterfaceCompliance(transceiver, profile);
}

void HwTransceiverUtils::verifyOpticsSettings(
    const TransceiverInfo& transceiver) {
  auto settings = apache::thrift::can_throw(*transceiver.settings_ref());

  if (*transceiver.transceiver_ref() == TransceiverType::QSFP) {
    // Disable low power mode
    EXPECT_TRUE(
        *settings.powerControl_ref() == PowerControlState::POWER_OVERRIDE ||
        *settings.powerControl_ref() == PowerControlState::HIGH_POWER_OVERRIDE);
    EXPECT_EQ(*settings.cdrTx_ref(), FeatureState::ENABLED);
    EXPECT_EQ(*settings.cdrRx_ref(), FeatureState::ENABLED);

    for (auto& mediaLane :
         apache::thrift::can_throw(*settings.mediaLaneSettings_ref())) {
      EXPECT_FALSE(mediaLane.txSquelch_ref().value());
    }

    for (auto& hostLane :
         apache::thrift::can_throw(*settings.hostLaneSettings_ref())) {
      EXPECT_FALSE(hostLane.rxSquelch_ref().value());
    }
  }

  for (auto& mediaLane :
       apache::thrift::can_throw(*settings.mediaLaneSettings_ref())) {
    EXPECT_FALSE(mediaLane.txDisable_ref().value());
  }
}

void HwTransceiverUtils::verifyMediaInterfaceCompliance(
    const TransceiverInfo& transceiver,
    cfg::PortProfileID profile) {
  auto settings = apache::thrift::can_throw(*transceiver.settings_ref());
  auto mgmtInterface = apache::thrift::can_throw(
      *transceiver.transceiverManagementInterface_ref());
  EXPECT_TRUE(
      mgmtInterface == TransceiverManagementInterface::SFF8472 ||
      mgmtInterface == TransceiverManagementInterface::SFF ||
      mgmtInterface == TransceiverManagementInterface::CMIS);
  auto mediaInterfaces =
      apache::thrift::can_throw(*settings.mediaInterface_ref());

  switch (profile) {
    case cfg::PortProfileID::PROFILE_10G_1_NRZ_NOFEC_OPTICAL:
      verify10gProfile(transceiver, mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_OPTICAL:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_OPTICAL:
      verify100gProfile(mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_40G_4_NRZ_NOFEC:
      // TODO
      break;

    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N:
    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_OPTICAL:
      verify200gProfile(mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL:
      verify400gProfile(mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER:
      verifyCopper100gProfile(transceiver, mediaInterfaces);
      break;

    default:
      throw FbossError(
          "Unhandled profile ", apache::thrift::util::enumNameSafe(profile));
  }
}

void HwTransceiverUtils::verify10gProfile(
    const TransceiverInfo& transceiver,
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::SFF8472);
  EXPECT_EQ(*transceiver.transceiver_ref(), TransceiverType::SFP);

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_EQ(
        *mediaId.media_ref()->ethernet10GComplianceCode_ref(),
        Ethernet10GComplianceCode::LR_10G);

    EXPECT_EQ(*mediaId.code_ref(), MediaInterfaceCode::LR_10G);
  }
}

void HwTransceiverUtils::verify100gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  for (const auto& mediaId : mediaInterfaces) {
    if (mgmtInterface == TransceiverManagementInterface::SFF) {
      auto specComplianceCode =
          *mediaId.media_ref()->extendedSpecificationComplianceCode_ref();
      EXPECT_TRUE(
          specComplianceCode == ExtendedSpecComplianceCode::CWDM4_100G ||
          specComplianceCode == ExtendedSpecComplianceCode::FR1_100G);
      EXPECT_TRUE(
          mediaId.code_ref() == MediaInterfaceCode::CWDM4_100G ||
          mediaId.code_ref() == MediaInterfaceCode::FR1_100G);
    } else if (mgmtInterface == TransceiverManagementInterface::CMIS) {
      EXPECT_EQ(
          *mediaId.media_ref()->smfCode_ref(),
          SMFMediaInterfaceCode::CWDM4_100G);
      EXPECT_EQ(*mediaId.code_ref(), MediaInterfaceCode::CWDM4_100G);
    }
  }
}

void HwTransceiverUtils::verify200gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::CMIS);

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_EQ(
        *mediaId.media_ref()->smfCode_ref(), SMFMediaInterfaceCode::FR4_200G);
    EXPECT_EQ(*mediaId.code_ref(), MediaInterfaceCode::FR4_200G);
  }
}

void HwTransceiverUtils::verify400gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::CMIS);

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(
        *mediaId.media_ref()->smfCode_ref() ==
            SMFMediaInterfaceCode::FR4_400G ||
        *mediaId.media_ref()->smfCode_ref() ==
            SMFMediaInterfaceCode::LR4_10_400G);
    EXPECT_TRUE(
        *mediaId.code_ref() == MediaInterfaceCode::FR4_400G ||
        *mediaId.code_ref() == MediaInterfaceCode::LR4_400G_10KM);
  }
}

void HwTransceiverUtils::verifyCopper100gProfile(
    const TransceiverInfo& transceiver,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(
      TransmitterTechnology::COPPER,
      *(transceiver.cable_ref().value_or({}).transmitterTech_ref()));

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(*mediaId.code_ref() == MediaInterfaceCode::CR4_100G);
  }
}

} // namespace facebook::fboss::utility
