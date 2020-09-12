/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/lib/fpga/FbFpga.h"

namespace facebook::fboss {
/**
 * FPGA device in Minipack platform.
 * NOTE: In FPGA, PIM number always starts from 1
 */
class MinipackFpga : public FbFpga {
 public:
  static constexpr auto kNumPortsPerPim = 16;

  MinipackFpga();
  static std::shared_ptr<MinipackFpga> getInstance();

  bool isQsfpPresent(uint8_t pim, int qsfp);
  std::array<bool, kNumPortsPerPim> scanQsfpPresence(uint8_t pim);
  void ensureQsfpOutOfReset(uint8_t pim, int qsfp);

  /* Trigger the QSFP hard reset of a given port on a given line card (PIM)
   * by toggling the FPGA bit. This function is for minipack line cards:
   * 16q and 4dd
   */
  void triggerQsfpHardReset(uint8_t pim, int qsfp);

  // This function will bring all the transceivers out of reset.
  void clearAllTransceiverReset();
};

} // namespace facebook::fboss
