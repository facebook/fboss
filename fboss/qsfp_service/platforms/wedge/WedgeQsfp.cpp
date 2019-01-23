/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "WedgeQsfp.h"
#include <folly/Conv.h>
#include <folly/Memory.h>
#include <folly/ScopeGuard.h>

#include <folly/logging/xlog.h>
#include "fboss/qsfp_service/StatsPublisher.h"

using namespace facebook::fboss;
using folly::MutableByteRange;
using std::make_unique;
using folly::StringPiece;
using std::unique_ptr;

namespace facebook { namespace fboss {

WedgeQsfp::WedgeQsfp(int module, TransceiverI2CApi* wedgeI2CBus)
    : module_(module), threadSafeI2CBus_(wedgeI2CBus) {
  moduleName_ = folly::to<std::string>(module);
}

WedgeQsfp::~WedgeQsfp() {
}

// Note that the module_ starts at 0, but the I2C bus module
// assumes that QSFP module numbers extend from 1 to 16.
//
bool WedgeQsfp::detectTransceiver() {
  return threadSafeI2CBus_->isPresent(module_ + 1);
}

void WedgeQsfp::ensureOutOfReset() {
  threadSafeI2CBus_->ensureOutOfReset(module_ + 1);
}

int WedgeQsfp::readTransceiver(int dataAddress, int offset,
                               int len, uint8_t* fieldValue) {
  try {
    SCOPE_EXIT {
      wedgeQsfpstats_.updateReadDownTime();
    };
    SCOPE_FAIL {
      StatsPublisher::bumpReadFailure();
    };
    SCOPE_SUCCESS {
      wedgeQsfpstats_.recordReadSuccess();
    };
    threadSafeI2CBus_->moduleRead(module_ + 1, dataAddress, offset, len,
                                  fieldValue);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Read from transceiver " << module_ << " at offset " << offset
              << " with length " << len << " failed: " << ex.what();
    throw;
  }
  return len;
}

int WedgeQsfp::writeTransceiver(
    int dataAddress,
    int offset,
    int len,
    uint8_t* fieldValue) {
  try {
    SCOPE_EXIT {
      wedgeQsfpstats_.updateWriteDownTime();
    };
    SCOPE_FAIL {
      StatsPublisher::bumpWriteFailure();
    };
    SCOPE_SUCCESS {
      wedgeQsfpstats_.recordWriteSuccess();
    };
    threadSafeI2CBus_->moduleWrite(
        module_ + 1, dataAddress, offset, len, fieldValue);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Write to transceiver " << module_ << " at offset " << offset
              << " with length " << len
              << " failed: " << folly::exceptionStr(ex);
    throw;
  }
  return len;
}

folly::StringPiece WedgeQsfp::getName() {
  return moduleName_;
}

int WedgeQsfp::getNum() const {
  return module_;
}

folly::Optional<TransceiverStats> WedgeQsfp::getTransceiverStats() {
  auto result = folly::Optional<TransceiverStats>();
  result.assign(wedgeQsfpstats_.getStats());
  return result;
}
}}
