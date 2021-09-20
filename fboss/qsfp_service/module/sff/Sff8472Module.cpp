// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/sff/Sff8472Module.h"

#include <string>
#include "fboss/agent/FbossError.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"
#include "fboss/qsfp_service/module/sff/Sff8472FieldInfo.h"

#include <folly/logging/xlog.h>

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace {}

namespace facebook {
namespace fboss {

enum Sff472Pages {
  LOWER,
};

static std::map<Ethernet10GComplianceCode, MediaInterfaceCode>
    mediaInterfaceMapping = {
        {Ethernet10GComplianceCode::LR_10G, MediaInterfaceCode::LR_10G},
};

// As per SFF-8472
static Sff8472FieldInfo::Sff8472FieldMap sfpFields = {
    // Address A0h fields
    {Sff8472Field::IDENTIFIER,
     {TransceiverI2CApi::ADDR_QSFP, Sff472Pages::LOWER, 0, 1}},
    {Sff8472Field::ETHERNET_10G_COMPLIANCE_CODE,
     {TransceiverI2CApi::ADDR_QSFP, Sff472Pages::LOWER, 3, 1}},

    // Address A2h fields
    {Sff8472Field::ALARM_WARNING_THRESHOLDS,
     {TransceiverI2CApi::ADDR_QSFP_A2, Sff472Pages::LOWER, 0, 40}},
};

Sff8472Module::Sff8472Module(
    TransceiverManager* transceiverManager,
    std::unique_ptr<TransceiverImpl> qsfpImpl,
    unsigned int portsPerTransceiver)
    : QsfpModule(transceiverManager, std::move(qsfpImpl), portsPerTransceiver) {
}

Sff8472Module::~Sff8472Module() {}

void Sff8472Module::updateQsfpData(bool allPages) {
  // expects the lock to be held
  if (!present_) {
    return;
  }
  try {
    XLOG(DBG2) << "Performing " << ((allPages) ? "full" : "partial")
               << " sfp data cache refresh for transceiver "
               << folly::to<std::string>(qsfpImpl_->getName());
    qsfpImpl_->readTransceiver(
        TransceiverI2CApi::ADDR_QSFP_A2, 0, sizeof(a2LowerPage_), a2LowerPage_);
    lastRefreshTime_ = std::time(nullptr);
    dirty_ = false;

    if (!allPages) {
      // Only address A2h lower page0 has diagnostic data that changes very
      // often. Avoid reading the static data frequently.
      return;
    }

    qsfpImpl_->readTransceiver(
        TransceiverI2CApi::ADDR_QSFP, 0, sizeof(a0LowerPage_), a0LowerPage_);
  } catch (const std::exception& ex) {
    // No matter what kind of exception throws, we need to set the dirty_ flag
    // to true.
    dirty_ = true;
    XLOG(ERR) << "Error update data for transceiver:"
              << folly::to<std::string>(qsfpImpl_->getName()) << ": "
              << ex.what();
    throw;
  }
}

TransceiverSettings Sff8472Module::getTransceiverSettingsInfo() {
  TransceiverSettings settings = TransceiverSettings();
  settings.mediaInterface_ref() =
      std::vector<MediaInterfaceId>(numMediaLanes());
  if (!getMediaInterfaceId(*(settings.mediaInterface_ref()))) {
    settings.mediaInterface_ref().reset();
  }

  return settings;
}

bool Sff8472Module::getMediaInterfaceId(
    std::vector<MediaInterfaceId>& mediaInterface) {
  CHECK_EQ(mediaInterface.size(), numMediaLanes());

  // TODO: Read this from the module
  auto ethernet10GCompliance = Ethernet10GComplianceCode::LR_10G;
  for (int lane = 0; lane < mediaInterface.size(); lane++) {
    mediaInterface[lane].lane_ref() = lane;
    MediaInterfaceUnion media;
    media.ethernet10GComplianceCode_ref() = ethernet10GCompliance;
    if (auto it = mediaInterfaceMapping.find(ethernet10GCompliance);
        it != mediaInterfaceMapping.end()) {
      mediaInterface[lane].code_ref() = it->second;
    } else {
      XLOG(ERR) << folly::sformat(
          "Module {:s}, Unable to find MediaInterfaceCode for {:s}",
          qsfpImpl_->getName(),
          apache::thrift::util::enumNameSafe(ethernet10GCompliance));
      mediaInterface[lane].code_ref() = MediaInterfaceCode::UNKNOWN;
    }
    mediaInterface[lane].media_ref() = media;
  }

  return true;
}

} // namespace fboss
} // namespace facebook
