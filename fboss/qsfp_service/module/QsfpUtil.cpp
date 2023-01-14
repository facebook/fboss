// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/QsfpUtil.h"
#include <folly/logging/xlog.h>
#include <sysexits.h>

namespace facebook::fboss {

QsfpUtil::QsfpUtil(
    ReadViaServiceFn readFn,
    WriteViaServiceFn writeFn,
    folly::EventBase* evb)
    : readViaServiceFn_(readFn), writeViaServiceFn_(writeFn), evb_(evb) {}

QsfpUtil::QsfpUtil(ReadViaDirectIoFn readFn, WriteViaDirectIoFn writeFn)
    : readViaDirectIoFn_(readFn), writeViaDirectIoFn_(writeFn) {
  directAccess_ = true;
}

} // namespace facebook::fboss
