// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/FbFpgaPimQsfpController.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

FbFpgaPimQsfpController::FbFpgaPimQsfpController(
    std::unique_ptr<FpgaMemoryRegion> io,
    unsigned int portsPerPim)
    : memoryRegion_(std::move(io)),
      portsPerPim_(portsPerPim),
      registerOffsetToMutex_(setupRegisterOffsetToMutex()) {}

std::unordered_map<uint32_t, std::unique_ptr<folly::SharedMutex>>
FbFpgaPimQsfpController::setupRegisterOffsetToMutex() {
  std::unordered_map<uint32_t, std::unique_ptr<folly::SharedMutex>>
      registerOffsetToMutex;
  registerOffsetToMutex.emplace(
      kFacebookFpgaQsfpPresentRegOffset,
      std::make_unique<folly::SharedMutex>());
  registerOffsetToMutex.emplace(
      kFacebookFpgaQsfpResetRegOffset, std::make_unique<folly::SharedMutex>());
  return registerOffsetToMutex;
}

folly::SharedMutex& FbFpgaPimQsfpController::getMutex(
    uint32_t registerOffset) const {
  if (const auto& mutexItr = registerOffsetToMutex_.find(registerOffset);
      mutexItr != registerOffsetToMutex_.end()) {
    return *(mutexItr->second.get());
  }
  throw FbossError("Unrecoginized register offset:", registerOffset);
}

bool FbFpgaPimQsfpController::isQsfpPresent(int qsfp) {
  folly::SharedMutex::ReadHolder g(getMutex(kFacebookFpgaQsfpPresentRegOffset));
  uint32_t qsfpPresentReg =
      memoryRegion_->read(kFacebookFpgaQsfpPresentRegOffset);
  // From the lower end, each bit of this register represent the presence of a
  // QSFP.
  XLOG(DBG5) << folly::format("qsfpPresentReg value:{:#x}", qsfpPresentReg);
  return (qsfpPresentReg >> qsfp) & 1;
}

std::vector<bool> FbFpgaPimQsfpController::scanQsfpPresence() {
  folly::SharedMutex::ReadHolder g(getMutex(kFacebookFpgaQsfpPresentRegOffset));
  uint32_t qsfpPresentReg =
      memoryRegion_->read(kFacebookFpgaQsfpPresentRegOffset);
  // From the lower end, each bit of this register represent the presence of a
  // QSFP.
  XLOG(DBG5) << folly::format("qsfpPresentReg value:{:#x}", qsfpPresentReg);
  std::vector<bool> qsfpPresence;
  for (int i = 0; i < portsPerPim_; i++) {
    qsfpPresence.push_back((qsfpPresentReg >> i) & 1);
  }
  return qsfpPresence;
}

// Trigger the QSFP hard reset for a given QSFP module.
void FbFpgaPimQsfpController::triggerQsfpHardReset(unsigned int port) {
  folly::SharedMutex::WriteHolder g(getMutex(kFacebookFpgaQsfpResetRegOffset));
  uint32_t originalResetReg =
      memoryRegion_->read(kFacebookFpgaQsfpResetRegOffset);

  // 1: hold QSFP in reset state
  // 0: release QSFP reset for normal operation

  // Hold the QSFP in reset state
  uint32_t newResetReg = (0x1 << port) | originalResetReg;

  XLOG(INFO) << folly::format(
      "For {}, port{:d}, old QsfpResetReg value:{:#x}, new value:{:#x}",
      memoryRegion_->getName(),
      port,
      originalResetReg,
      newResetReg);
  memoryRegion_->write(kFacebookFpgaQsfpResetRegOffset, newResetReg);

  // Wait for 10ms
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  // Release the QSFP reset
  newResetReg = ~(0x1 << port) & originalResetReg;
  memoryRegion_->write(kFacebookFpgaQsfpResetRegOffset, newResetReg);
}

void FbFpgaPimQsfpController::ensureQsfpOutOfReset(int qsfp) {
  folly::SharedMutex::WriteHolder g(getMutex(kFacebookFpgaQsfpResetRegOffset));
  uint32_t currentResetReg =
      memoryRegion_->read(kFacebookFpgaQsfpResetRegOffset);
  // 1 to hold QSFP reset active. 0 to release QSFP reset.
  uint32_t newResetReg = ~(0x1 << qsfp) & currentResetReg;
  if (currentResetReg != newResetReg) {
    XLOG(DBG5) << folly::format(
        "For {}, port{:d}, old QsfpResetReg value:{:#x}, new value:{:#x}",
        memoryRegion_->getName(),
        qsfp,
        currentResetReg,
        newResetReg);
    memoryRegion_->write(kFacebookFpgaQsfpResetRegOffset, newResetReg);
  }
}

/* This function will bring all the transceivers out of reset. Just clear the
 * reset bits of all the transceivers through FPGA.
 */
void FbFpgaPimQsfpController::clearAllTransceiverReset() {
  folly::SharedMutex::WriteHolder g(getMutex(kFacebookFpgaQsfpResetRegOffset));
  XLOG(DBG5) << "Clearing all transceiver out of reset.";
  // For each bit, 1 to hold QSFP reset active. 0 to release QSFP reset.
  memoryRegion_->write(kFacebookFpgaQsfpResetRegOffset, 0x0);
}

} // namespace facebook::fboss
