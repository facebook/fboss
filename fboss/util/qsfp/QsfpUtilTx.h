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
      DirectI2cInfo i2cInfo,
      const std::vector<std::string>& portNames,
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
  bool setSffTxDisableDirect(const std::vector<std::string>& sffPortNames);
  bool setCmisTxDisableDirect(const std::vector<std::string>& cmisPortNames);
  int setTxDisableViaService();
  bool setTxDisableViaServiceHelper(const std::vector<std::string>& portNames);

  static const uint8_t maxSffChannels = 4;
  static const uint8_t maxCmisChannels = 8;

  TransceiverI2CApi* bus_;
  TransceiverManager* wedgeManager_;
  std::vector<std::string> allPortNames_;
  std::vector<std::string> sffPortNames_;
  std::vector<std::string> cmisPortNames_;
  folly::EventBase& evb_;
  bool disableTx_;
};

} // namespace facebook::fboss
