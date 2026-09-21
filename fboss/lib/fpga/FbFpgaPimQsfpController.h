// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/HwMemoryRegion.h"

#include <folly/SharedMutex.h>

namespace facebook::fboss {

class FbFpgaPimQsfpController {
 public:
  FbFpgaPimQsfpController(
      std::unique_ptr<FpgaMemoryRegion> io,
      unsigned int portsPerPim);
  ~FbFpgaPimQsfpController() {}

  bool isQsfpPresent(int qsfp);

  std::vector<bool> scanQsfpPresence();

  // Trigger the QSFP hard reset for a given QSFP module.
  void triggerQsfpHardReset(unsigned int port);

  // Some platform may put transceiver into reset by default.
  // This function is to make sure getting transceiver out of
  // reset before accessing it.
  void ensureQsfpOutOfReset(int qsfp);

  // This function will bring all the transceivers out of reset.
  void clearAllTransceiverReset();

 private:
  std::unordered_map<uint32_t, std::unique_ptr<folly::SharedMutex>>
  setupRegisterOffsetToMutex();
  folly::SharedMutex& getMutex(uint32_t registerOffset) const;

  const uint32_t kFacebookFpgaQsfpPresentRegOffset = 0x8;
  const uint32_t kFacebookFpgaQsfpResetRegOffset = 0x30;

  std::unique_ptr<FpgaMemoryRegion> memoryRegion_;
  unsigned int portsPerPim_;
  // Because all the qsfp with the same controller is sharing the same offset,
  // and use different bits to read the value, it will be racy for different
  // qsfps try to read the same register offset value and then write at the same
  // time. Use mutex to protect the race condition for each register offset
  const std::unordered_map<uint32_t, std::unique_ptr<folly::SharedMutex>>
      registerOffsetToMutex_;
};

} // namespace facebook::fboss
