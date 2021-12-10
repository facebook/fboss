/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <folly/io/async/EventBase.h>
#include <vector>
#include "fboss/util/wedge_qsfp_util.h"

namespace facebook::fboss {

class QsfpUtilTx {
 public:
  QsfpUtilTx(
      TransceiverI2CApi* bus,
      const std::vector<unsigned int>& ports,
      folly::EventBase& evb);
  ~QsfpUtilTx() {}
  int setTxDisable();

  // Forbidden copy constructor and assignment operator
  QsfpUtilTx(QsfpUtilTx const&) = delete;
  QsfpUtilTx& operator=(QsfpUtilTx const&) = delete;

 private:
  void setChannelDisable(
      const TransceiverManagementInterface moduleType,
      uint8_t& data);
  int setTxDisableDirect();
  bool setSffTxDisableDirect(unsigned int port);
  bool setCmisTxDisableDirect(unsigned int port);
  int setTxDisableViaService();
  bool setCmisTxDisableViaService(const std::vector<int32_t>& ports);
  bool setSffTxDisableViaService(const std::vector<int32_t>& ports);

  static const uint8_t maxSffChannels = 4;
  static const uint8_t maxCmisChannels = 8;

  TransceiverI2CApi* bus_;
  const std::vector<unsigned int>& ports_;
  folly::EventBase& evb_;
  bool disableTx_;
};

} // namespace facebook::fboss
