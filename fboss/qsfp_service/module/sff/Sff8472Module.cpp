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

} // namespace fboss
} // namespace facebook
