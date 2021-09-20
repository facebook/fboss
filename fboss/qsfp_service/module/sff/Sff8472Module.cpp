// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/sff/Sff8472Module.h"

#include <assert.h>
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

// As per SFF-8472
static Sff8472FieldInfo::Sff8472FieldMap sfpFields = {
    // Address A0h fields
    {Sff8472Field::IDENTIFIER,
     {TransceiverI2CApi::ADDR_QSFP, Sff472Pages::LOWER, 0, 1}},

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
    setQsfpFlatMem();

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

} // namespace fboss
} // namespace facebook
