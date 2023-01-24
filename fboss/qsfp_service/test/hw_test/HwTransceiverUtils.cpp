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
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss::utility {

void HwTransceiverUtils::verifyTransceiverSettings(
    const TransceiverInfo& transceiver,
    cfg::PortProfileID profile) {
  auto id = *transceiver.port();
  if (!*transceiver.present()) {
    XLOG(INFO) << " Skip verifying: " << id << ", not present";
    return;
  }

  XLOG(INFO) << " Verifying: " << id;
  // Only testing QSFP and SFP transceivers right now
  EXPECT_TRUE(
      *transceiver.transceiver() == TransceiverType::QSFP ||
      *transceiver.transceiver() == TransceiverType::SFP);

  if (TransmitterTechnology::COPPER ==
      *(transceiver.cable().value_or({}).transmitterTech())) {
    XLOG(INFO) << " Skip verifying optics settings: " << *transceiver.port()
               << ", for copper cable";
  } else {
    verifyOpticsSettings(transceiver, profile);
  }

  verifyMediaInterfaceCompliance(transceiver, profile);

  verifyDataPathEnabled(transceiver);
}

void HwTransceiverUtils::verifyOpticsSettings(
    const TransceiverInfo& transceiver,
    cfg::PortProfileID profile) {
  auto settings = apache::thrift::can_throw(*transceiver.settings());

  for (auto& mediaLane :
       apache::thrift::can_throw(*settings.mediaLaneSettings())) {
    EXPECT_FALSE(*mediaLane.txDisable())
        << "Transceiver:" << *transceiver.port()
        << ", Lane=" << *mediaLane.lane()
        << " txDisable doesn't match expected";
    if (*transceiver.transceiver() == TransceiverType::QSFP) {
      EXPECT_EQ(
          *mediaLane.txSquelch(),
          profile ==
              cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL)
          << "Transceiver:" << *transceiver.port()
          << ", Lane=" << *mediaLane.lane()
          << " txSquelch doesn't match expected";
    }
  }

  if (*transceiver.transceiver() == TransceiverType::QSFP) {
    // Disable low power mode
    EXPECT_TRUE(
        *settings.powerControl() == PowerControlState::POWER_OVERRIDE ||
        *settings.powerControl() == PowerControlState::HIGH_POWER_OVERRIDE);
    EXPECT_EQ(*settings.cdrTx(), FeatureState::ENABLED);
    EXPECT_EQ(*settings.cdrRx(), FeatureState::ENABLED);

    for (auto& hostLane :
         apache::thrift::can_throw(*settings.hostLaneSettings())) {
      EXPECT_EQ(
          *hostLane.rxSquelch(),
          profile ==
              cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL)
          << "Transceiver:" << *transceiver.port()
          << ", Lane=" << *hostLane.lane()
          << " rxSquelch doesn't match expected";
    }
  }
}

void HwTransceiverUtils::verifyMediaInterfaceCompliance(
    const TransceiverInfo& transceiver,
    cfg::PortProfileID profile) {
  auto settings = apache::thrift::can_throw(*transceiver.settings());
  auto mgmtInterface =
      apache::thrift::can_throw(*transceiver.transceiverManagementInterface());
  EXPECT_TRUE(
      mgmtInterface == TransceiverManagementInterface::SFF8472 ||
      mgmtInterface == TransceiverManagementInterface::SFF ||
      mgmtInterface == TransceiverManagementInterface::CMIS);
  auto mediaInterfaces = apache::thrift::can_throw(*settings.mediaInterface());

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
    case cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_OPTICAL:
      verify200gProfile(mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_400G_8_PAM4_RS544X2N_OPTICAL:
      verify400gProfile(mgmtInterface, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_100G_4_NRZ_RS528_COPPER:
    case cfg::PortProfileID::PROFILE_100G_4_NRZ_CL91_COPPER:
      verifyCopper100gProfile(transceiver, mediaInterfaces);
      break;

    case cfg::PortProfileID::PROFILE_200G_4_PAM4_RS544X2N_COPPER:
    case cfg::PortProfileID::PROFILE_53POINT125G_1_PAM4_RS545_COPPER:
      verifyCopper200gProfile(transceiver, mediaInterfaces);
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
  EXPECT_EQ(*transceiver.transceiver(), TransceiverType::SFP);

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_EQ(
        *mediaId.media()->ethernet10GComplianceCode_ref(),
        Ethernet10GComplianceCode::LR_10G);

    EXPECT_EQ(*mediaId.code(), MediaInterfaceCode::LR_10G);
  }
}

void HwTransceiverUtils::verify100gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  for (const auto& mediaId : mediaInterfaces) {
    if (mgmtInterface == TransceiverManagementInterface::SFF) {
      auto specComplianceCode =
          *mediaId.media()->extendedSpecificationComplianceCode_ref();
      EXPECT_TRUE(
          specComplianceCode == ExtendedSpecComplianceCode::CWDM4_100G ||
          specComplianceCode == ExtendedSpecComplianceCode::FR1_100G ||
          specComplianceCode == ExtendedSpecComplianceCode::CR4_100G);
      EXPECT_TRUE(
          mediaId.code() == MediaInterfaceCode::CWDM4_100G ||
          mediaId.code() == MediaInterfaceCode::FR1_100G ||
          mediaId.code() == MediaInterfaceCode::CR4_100G);
    } else if (mgmtInterface == TransceiverManagementInterface::CMIS) {
      EXPECT_EQ(
          *mediaId.media()->smfCode_ref(), SMFMediaInterfaceCode::CWDM4_100G);
      EXPECT_EQ(*mediaId.code(), MediaInterfaceCode::CWDM4_100G);
    }
  }
}

void HwTransceiverUtils::verify200gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::CMIS);

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_EQ(*mediaId.media()->smfCode_ref(), SMFMediaInterfaceCode::FR4_200G);
    EXPECT_EQ(*mediaId.code(), MediaInterfaceCode::FR4_200G);
  }
}

void HwTransceiverUtils::verify400gProfile(
    const TransceiverManagementInterface mgmtInterface,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(mgmtInterface, TransceiverManagementInterface::CMIS);

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(
        *mediaId.media()->smfCode_ref() == SMFMediaInterfaceCode::FR4_400G ||
        *mediaId.media()->smfCode_ref() == SMFMediaInterfaceCode::LR4_10_400G);
    EXPECT_TRUE(
        *mediaId.code() == MediaInterfaceCode::FR4_400G ||
        *mediaId.code() == MediaInterfaceCode::LR4_400G_10KM);
  }
}

void HwTransceiverUtils::verifyCopper100gProfile(
    const TransceiverInfo& transceiver,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(
      TransmitterTechnology::COPPER,
      *(transceiver.cable().value_or({}).transmitterTech()));

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(*mediaId.code() == MediaInterfaceCode::CR4_100G);
  }
}

void HwTransceiverUtils::verifyCopper200gProfile(
    const TransceiverInfo& transceiver,
    const std::vector<MediaInterfaceId>& mediaInterfaces) {
  EXPECT_EQ(
      TransmitterTechnology::COPPER,
      *(transceiver.cable().value_or({}).transmitterTech()));

  for (const auto& mediaId : mediaInterfaces) {
    EXPECT_TRUE(*mediaId.code() == MediaInterfaceCode::CR4_200G);
  }
}

void HwTransceiverUtils::verifyDataPathEnabled(
    const TransceiverInfo& transceiver) {
  auto mgmtInterface = transceiver.transceiverManagementInterface();
  if (!mgmtInterface) {
    throw FbossError(
        "Transceiver:",
        *transceiver.port(),
        " is missing transceiverManagementInterface");
  }
  // Right now, we only reset data path for CMIS module
  if (*mgmtInterface == TransceiverManagementInterface::CMIS) {
    // All lanes should have `false` Deinit
    if (auto hostLaneSignals = transceiver.hostLaneSignals()) {
      for (const auto& laneSignals : *hostLaneSignals) {
        EXPECT_TRUE(laneSignals.dataPathDeInit().has_value())
            << "Transceiver:" << *transceiver.port()
            << ", Lane=" << *laneSignals.lane()
            << " is missing dataPathDeInit signal";
        EXPECT_FALSE(*laneSignals.dataPathDeInit())
            << "Transceiver:" << *transceiver.port()
            << ", Lane=" << *laneSignals.lane()
            << " dataPathDeInit doesn't match expected";
      }
    } else {
      throw FbossError(
          "Transceiver:", *transceiver.port(), " is missing hostLaneSignals");
    }
  }
}

void HwTransceiverUtils::verifyDiagsCapability(
    const TransceiverInfo& transceiver,
    std::optional<DiagsCapability> diagsCapability,
    bool skipCheckingIndividualCapability) {
  auto mgmtInterface = transceiver.transceiverManagementInterface();
  if (!mgmtInterface) {
    throw FbossError(
        "Transceiver:",
        *transceiver.port(),
        " is missing transceiverManagementInterface");
  }
  auto mediaIntfCode = transceiver.moduleMediaInterface();
  if (!mediaIntfCode) {
    throw FbossError(
        "Transceiver:",
        *transceiver.port(),
        " is missing moduleMediaInterface");
  }
  XLOG(INFO) << "Verifying DiagsCapability for Transceiver="
             << *transceiver.port() << ", mgmtInterface="
             << apache::thrift::util::enumNameSafe(*mgmtInterface)
             << ", moduleMediaInterface="
             << apache::thrift::util::enumNameSafe(*mediaIntfCode);

  switch (*mgmtInterface) {
    case TransceiverManagementInterface::CMIS:
      EXPECT_TRUE(diagsCapability.has_value());
      if (!skipCheckingIndividualCapability) {
        EXPECT_TRUE(*diagsCapability->diagnostics());
        // Only 400G CMIS supports VDM
        EXPECT_EQ(
            *diagsCapability->vdm(),
            (*mediaIntfCode == MediaInterfaceCode::FR4_400G ||
             *mediaIntfCode == MediaInterfaceCode::LR4_400G_10KM));
        EXPECT_TRUE(*diagsCapability->cdb());
        EXPECT_TRUE(*diagsCapability->prbsLine());
        EXPECT_TRUE(*diagsCapability->prbsSystem());
        EXPECT_TRUE(*diagsCapability->loopbackLine());
        EXPECT_TRUE(*diagsCapability->loopbackSystem());
      }
      return;
    case TransceiverManagementInterface::SFF:
      // Only FR1_100G has diagsCapability
      if (*mediaIntfCode == MediaInterfaceCode::FR1_100G) {
        EXPECT_TRUE(diagsCapability.has_value());
        if (!skipCheckingIndividualCapability) {
          EXPECT_TRUE(*diagsCapability->prbsLine());
          EXPECT_TRUE(*diagsCapability->prbsSystem());
        }
      } else {
        EXPECT_FALSE(diagsCapability.has_value());
      }
      return;
    case TransceiverManagementInterface::SFF8472:
    case TransceiverManagementInterface::NONE:
      EXPECT_FALSE(diagsCapability.has_value());
      return;
  }
  throw FbossError(
      "Transceiver:",
      *transceiver.port(),
      " is using unrecognized transceiverManagementInterface:",
      apache::thrift::util::enumNameSafe(*mgmtInterface));
}
} // namespace facebook::fboss::utility
