// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/sff/Sff8472Module.h"

#include <assert.h>
#include <string>
#include "fboss/agent/FbossError.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"

#include <folly/logging/xlog.h>

#include <thrift/lib/cpp/util/EnumUtils.h>

namespace {}

namespace facebook {
namespace fboss {

Sff8472Module::Sff8472Module(
    TransceiverManager* transceiverManager,
    std::unique_ptr<TransceiverImpl> qsfpImpl,
    unsigned int portsPerTransceiver)
    : QsfpModule(transceiverManager, std::move(qsfpImpl), portsPerTransceiver) {
}

Sff8472Module::~Sff8472Module() {}

} // namespace fboss
} // namespace facebook
