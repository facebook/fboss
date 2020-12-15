// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/HwMemoryRegion.h"

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
  std::unique_ptr<FpgaMemoryRegion> memoryRegion_;
  unsigned int portsPerPim_;
};

} // namespace facebook::fboss
