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
#include <folly/Random.h>
#include <folly/ScopeGuard.h>

#include <folly/logging/xlog.h>
#include "fboss/qsfp_service/StatsPublisher.h"
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/module/QsfpModule.h"

#include "fboss/qsfp_service/module/cmis/gen-cpp2/cmis_types.h"

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

WedgeQsfp::WedgeQsfp(
    int module,
    TransceiverI2CApi* wedgeI2CBus,
    TransceiverManager* const tcvrManager,
    std::unique_ptr<I2cLogBuffer> logBuffer)
    : module_(module),
      threadSafeI2CBus_(wedgeI2CBus),
      tcvrManager_(tcvrManager),
      logBuffer_(std::move(logBuffer)) {
  moduleName_ = folly::to<std::string>(module);
}

WedgeQsfp::~WedgeQsfp() {}

int WedgeQsfp::readTransceiver(
    const TransceiverAccessParameter& param,
    uint8_t* fieldValue,
    const int field) {
  auto offset = param.offset;
  auto len = param.len;
  ioStatsRecorder_.recordReadAttempted();
  try {
    SCOPE_EXIT {
      ioStatsRecorder_.updateReadDownTime();
    };
    SCOPE_FAIL {
      ioStatsRecorder_.recordReadFailed();
      StatsPublisher::bumpReadFailure();
    };
    SCOPE_SUCCESS {
      ioStatsRecorder_.recordReadSuccess();
    };
    generateIOErrorForTest("readTransceiver()");
    threadSafeI2CBus_->moduleRead(module_ + 1, param, fieldValue);
    if (logBuffer_) {
      logBuffer_->log(param, field, fieldValue, I2cLogBuffer::Operation::Read);
    }
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Read from transceiver " << module_ << " at offset " << offset
              << " with length " << len << " failed: " << ex.what();
    if (logBuffer_) {
      logBuffer_->log(
          param,
          field,
          fieldValue,
          I2cLogBuffer::Operation::Read,
          /*success*/ false);
    }
    throw;
  }
  return len;
}

int WedgeQsfp::writeTransceiver(
    const TransceiverAccessParameter& param,
    const uint8_t* fieldValue,
    uint64_t delay,
    const int field) {
  auto offset = param.offset;
  auto len = param.len;
  ioStatsRecorder_.recordWriteAttempted();
  try {
    SCOPE_EXIT {
      ioStatsRecorder_.updateWriteDownTime();
    };
    SCOPE_FAIL {
      ioStatsRecorder_.recordWriteFailed();
      StatsPublisher::bumpWriteFailure();
    };
    SCOPE_SUCCESS {
      ioStatsRecorder_.recordWriteSuccess();
    };
    generateIOErrorForTest("writeTransceiver()");
    threadSafeI2CBus_->moduleWrite(module_ + 1, param, fieldValue);
    if (logBuffer_) {
      logBuffer_->log(param, field, fieldValue, I2cLogBuffer::Operation::Write);
    }
    // Intel transceiver require some delay for every write.
    // So in the case of writing succeeded, we wait for 20ms.
    // Also this works because we do not write more than 1 byte for now.
    usleep(delay);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Write to transceiver " << module_ << " at offset " << offset
              << " with length " << len
              << " failed: " << folly::exceptionStr(ex);
    if (logBuffer_) {
      logBuffer_->log(
          param,
          field,
          fieldValue,
          I2cLogBuffer::Operation::Write,
          /*success*/ false);
    }
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
  auto result = TransceiverStats();
  auto ioStats = ioStatsRecorder_.getStats();
  // Once TransceiverStats is deprecated in favor of IOStats, remove the below
  // conversion and directly return ioStats
  result.numReadAttempted() = ioStats.numReadAttempted().value();
  result.numReadFailed() = ioStats.numReadFailed().value();
  result.numWriteAttempted() = ioStats.numWriteAttempted().value();
  result.numWriteFailed() = ioStats.numWriteFailed().value();
  result.readDownTime() = ioStats.readDownTime().value();
  result.writeDownTime() = ioStats.writeDownTime().value();
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
      readTransceiver(
          {TransceiverAccessParameter::ADDR_QSFP, 0, 1},
          buf.data(),
          // common enum to all tcvr types
          CAST_TO_INT(CmisField::MGMT_INTERFACE));
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
  readTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, kCommonModulePageReg, 1},
      &savedPage,
      // common enum to all tcvr types
      CAST_TO_INT(CmisField::PART_NUM));
  if (savedPage != page) {
    writeTransceiver(
        {TransceiverAccessParameter::ADDR_QSFP, kCommonModulePageReg, 1},
        &page,
        POST_I2C_WRITE_DELAY_US,
        // common enum to all tcvr types
        CAST_TO_INT(CmisField::PART_NUM));
  }

  readTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, partNoRegOffset, 16},
      partNo.data(),
      // common enum to all tcvr types
      CAST_TO_INT(CmisField::PART_NUM));
  if (savedPage != page) {
    writeTransceiver(
        {TransceiverAccessParameter::ADDR_QSFP, kCommonModulePageReg, 1},
        &savedPage,
        POST_I2C_WRITE_DELAY_US,
        // common enum to all tcvr types
        CAST_TO_INT(CmisField::PART_NUM));
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
  readTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, kCommonModuleFwVerReg, 2},
      fwVer.data(),
      // common enum to all tcvr types
      CAST_TO_INT(CmisField::FW_VERSION));
  return fwVer;
}

size_t WedgeQsfp::getI2cLogBufferCapacity() {
  if (logBuffer_) {
    return logBuffer_->getI2cLogBufferCapacity();
  }
  return 0;
}

void WedgeQsfp::setTcvrInfoInLog(
    const TransceiverManagementInterface& mgmtIf,
    const std::set<std::string>& portNames,
    const std::optional<FirmwareStatus>& status,
    const std::optional<Vendor>& vendor) {
  if (logBuffer_) {
    logBuffer_->setTcvrInfoInLog(mgmtIf, portNames, status, vendor);
  }
}

std::pair<size_t, size_t> WedgeQsfp::dumpTransceiverI2cLog() {
  std::pair<size_t, size_t> entries = {0, 0};
  if (logBuffer_) {
    try {
      entries = logBuffer_->dumpToFile();
    } catch (std::exception& ex) {
      XLOG(ERR) << fmt::format(
          "Failed to dump log for module{}: {:s}", module_, ex.what());
    }
  } else {
    XLOG(ERR) << fmt::format("Module has no I2C Log: {}", module_);
  }

  return entries;
}

} // namespace fboss
} // namespace facebook
