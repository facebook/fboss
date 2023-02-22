/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/platforms/wedge/WedgeQsfp.h"
#include <folly/Conv.h>
#include <folly/Memory.h>
#include <folly/Random.h>
#include <folly/ScopeGuard.h>

#include <folly/logging/xlog.h>
#include "fboss/qsfp_service/StatsPublisher.h"
#include "fboss/qsfp_service/module/QsfpModule.h"

using namespace facebook::fboss;
using folly::MutableByteRange;
using folly::StringPiece;
using std::make_unique;
using std::unique_ptr;

namespace {
constexpr uint8_t kCommonModulePageReg = 127;
constexpr uint8_t kCommonModuleBasePage = 0;
constexpr uint8_t kCmisModulePartNoReg = 148;
constexpr uint8_t kSffModulePartNoReg = 168;
constexpr uint8_t kCommonModuleFwVerReg = 39;

constexpr auto kNumInterfaceDetectionRetries = 5;
constexpr auto kInterfaceDetectionRetryMillis = 10;

DEFINE_int32(
    module_io_err_inject,
    0,
    "Probability of module IO error injection [0..100%]");

/*
 * generateIOErrorForTest
 *
 * This function will generate the I2c error based on the flag
 * module_io_err_inject. The FLAGS_module_io_err_inject indicates probability of
 * error in the range 0 to 100%. This function finds a random number in range
 * [1..100] and if that random number is in the range [1..desiredProbability]
 * then it generates i2c error. For 0% we don't generate any error and 100% will
 * lead to always generating the error. This function is only intended for the
 * test
 */
void generateIOErrorForTest(std::string functionName) {
  if (FLAGS_module_io_err_inject > 0 && FLAGS_module_io_err_inject <= 100) {
    int randomPlacement = folly::Random::rand32(1, 100);
    if (randomPlacement <= FLAGS_module_io_err_inject) {
      throw I2cError(folly::sformat("IO error injected in {:s}", functionName));
    }
  }
}
} // namespace

namespace facebook {
namespace fboss {

WedgeQsfp::WedgeQsfp(int module, TransceiverI2CApi* wedgeI2CBus)
    : module_(module), threadSafeI2CBus_(wedgeI2CBus) {
  moduleName_ = folly::to<std::string>(module);
}

WedgeQsfp::~WedgeQsfp() {}

// Note that the module_ starts at 0, but the I2C bus module
// assumes that QSFP module numbers extend from 1 to 16.
//
bool WedgeQsfp::detectTransceiver() {
  return threadSafeI2CBus_->isPresent(module_ + 1);
}

void WedgeQsfp::ensureOutOfReset() {
  threadSafeI2CBus_->ensureOutOfReset(module_ + 1);
}

int WedgeQsfp::readTransceiver(
    const TransceiverAccessParameter& param,
    uint8_t* fieldValue) {
  auto offset = param.offset;
  auto len = param.len;
  wedgeQsfpstats_.recordReadAttempted();
  try {
    SCOPE_EXIT {
      wedgeQsfpstats_.updateReadDownTime();
    };
    SCOPE_FAIL {
      wedgeQsfpstats_.recordReadFailed();
      StatsPublisher::bumpReadFailure();
    };
    SCOPE_SUCCESS {
      wedgeQsfpstats_.recordReadSuccess();
    };
    generateIOErrorForTest("readTransceiver()");
    threadSafeI2CBus_->moduleRead(module_ + 1, param, fieldValue);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Read from transceiver " << module_ << " at offset " << offset
              << " with length " << len << " failed: " << ex.what();
    throw;
  }
  return len;
}

int WedgeQsfp::writeTransceiver(
    const TransceiverAccessParameter& param,
    uint8_t* fieldValue) {
  auto offset = param.offset;
  auto len = param.len;
  wedgeQsfpstats_.recordWriteAttempted();
  try {
    SCOPE_EXIT {
      wedgeQsfpstats_.updateWriteDownTime();
    };
    SCOPE_FAIL {
      wedgeQsfpstats_.recordWriteFailed();
      StatsPublisher::bumpWriteFailure();
    };
    SCOPE_SUCCESS {
      wedgeQsfpstats_.recordWriteSuccess();
    };
    generateIOErrorForTest("writeTransceiver()");
    threadSafeI2CBus_->moduleWrite(module_ + 1, param, fieldValue);

    // Intel transceiver require some delay for every write.
    // So in the case of writing succeeded, we wait for 20ms.
    // Also this works because we do not write more than 1 byte for now.
    usleep(20000);
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

std::optional<TransceiverStats> WedgeQsfp::getTransceiverStats() {
  auto result = std::optional<TransceiverStats>();
  result = wedgeQsfpstats_.getStats();
  return result;
}

folly::EventBase* WedgeQsfp::getI2cEventBase() {
  return threadSafeI2CBus_->getEventBase(module_ + 1);
}

TransceiverManagementInterface WedgeQsfp::getTransceiverManagementInterface() {
  std::array<uint8_t, 1> buf;

  if (!threadSafeI2CBus_->isPresent(module_ + 1)) {
    XLOG(DBG3) << "Transceiver " << module_ << " not present";
    return TransceiverManagementInterface::NONE;
  }

  for (int i = 0; i < kNumInterfaceDetectionRetries; ++i) {
    try {
      threadSafeI2CBus_->moduleRead(
          module_ + 1, {TransceiverI2CApi::ADDR_QSFP, 0, 1}, buf.data());
      XLOG(DBG3) << folly::sformat(
          "Transceiver {:d}  identifier: {:#x}", module_, buf[0]);
      TransceiverManagementInterface modInterfaceType =
          QsfpModule::getTransceiverManagementInterface(buf[0], module_ + 1);

      if (modInterfaceType == TransceiverManagementInterface::UNKNOWN) {
        XLOG(WARNING) << folly::sformat(
            "Transceiver {:d} has unknown non-zero identifier: {:#x} defaulting to SFF",
            module_,
            buf[0]);
        return TransceiverManagementInterface::SFF;
      } else if (modInterfaceType != TransceiverManagementInterface::NONE) {
        return modInterfaceType;
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << "Transceiver " << module_
                << " failed to read management interface with exception: "
                << ex.what();
      /* sleep override */
      usleep(kInterfaceDetectionRetryMillis * 1000);
    }
  }
  return TransceiverManagementInterface::NONE;
}

folly::Future<TransceiverManagementInterface>
WedgeQsfp::futureGetTransceiverManagementInterface() {
  auto i2cEvb = getI2cEventBase();
  auto managementInterface = TransceiverManagementInterface::NONE;
  if (!i2cEvb) {
    try {
      managementInterface = getTransceiverManagementInterface();
    } catch (const std::exception& ex) {
      XLOG(ERR) << "WedgeQsfp " << getNum()
                << ": Error calling getTransceiverManagementInterface(): "
                << ex.what();
    }
    return managementInterface;
  }

  return via(i2cEvb).thenValue([&](auto&&) mutable {
    TransceiverManagementInterface mgmtInterface;
    try {
      mgmtInterface = this->getTransceiverManagementInterface();
    } catch (const std::exception& ex) {
      XLOG(ERR) << "WedgeQsfp " << this->getNum()
                << ": Error calling getTransceiverManagementInterface(): "
                << ex.what();
    }
    return mgmtInterface;
  });
}

std::array<uint8_t, 16> WedgeQsfp::getModulePartNo() {
  std::array<uint8_t, 16> partNo;
  uint8_t page = kCommonModuleBasePage, savedPage;

  auto moduleType = getTransceiverManagementInterface();
  uint8_t partNoRegOffset = (moduleType == TransceiverManagementInterface::CMIS)
      ? kCmisModulePartNoReg
      : kSffModulePartNoReg;

  // Read 16 byte part no from page 0 reg 148 for  CMIS and page 0 reg 168 for
  // SFF module. Restore the page in the end
  threadSafeI2CBus_->moduleRead(
      module_ + 1,
      {TransceiverI2CApi::ADDR_QSFP, kCommonModulePageReg, 1},
      &savedPage);
  if (savedPage != page) {
    threadSafeI2CBus_->moduleWrite(
        module_ + 1,
        {TransceiverI2CApi::ADDR_QSFP, kCommonModulePageReg, 1},
        &page);
  }

  threadSafeI2CBus_->moduleRead(
      module_ + 1,
      {TransceiverI2CApi::ADDR_QSFP, partNoRegOffset, 16},
      partNo.data());
  if (savedPage != page) {
    threadSafeI2CBus_->moduleWrite(
        module_ + 1,
        {TransceiverI2CApi::ADDR_QSFP, kCommonModulePageReg, 1},
        &savedPage);
  }

  return partNo;
}

std::array<uint8_t, 2> WedgeQsfp::getFirmwareVer() {
  std::array<uint8_t, 2> fwVer{0, 0};

  if (getTransceiverManagementInterface() !=
      TransceiverManagementInterface::CMIS) {
    XLOG(DBG3) << "Module is not CMIS type so firmware version can't be found";
    return fwVer;
  }

  // Read 2 byte firmware version from base page reg 39-40 for CMIS module
  threadSafeI2CBus_->moduleRead(
      module_ + 1,
      {TransceiverI2CApi::ADDR_QSFP, kCommonModuleFwVerReg, 2},
      fwVer.data());
  return fwVer;
}

} // namespace fboss
} // namespace facebook
