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
#include "fboss/agent/types.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/lib/usb/TransceiverPlatformApi.h"
#include "fboss/lib/usb/TransceiverPlatformI2cApi.h"
#include "fboss/qsfp_service/TransceiverManager.h"
#include "fboss/qsfp_service/lib/QsfpClient.h"

#include <memory>
#include <utility>

namespace facebook::fboss {

class QsfpUtil {
  using ReadViaServiceFn = std::function<void(
      const std::vector<int32_t>& ports,
      const TransceiverAccessParameter& param,
      folly::EventBase& evb,
      std::map<int32_t, ReadResponse>& response)>;

  using WriteViaServiceFn = std::function<void(
      const std::vector<int32_t>& ports,
      const TransceiverAccessParameter& param,
      uint8_t value,
      folly::EventBase& evb)>;

  using ReadViaDirectIoFn = std::function<void(
      unsigned int module,
      const TransceiverAccessParameter& param,
      uint8_t* buf)>;

  using WriteViaDirectIoFn = std::function<void(
      unsigned int module,
      const TransceiverAccessParameter& param,
      const uint8_t* buf)>;

 public:
  QsfpUtil(
      ReadViaServiceFn readFn,
      WriteViaServiceFn writeFn,
      folly::EventBase* evb);

  QsfpUtil(ReadViaDirectIoFn readFn, WriteViaDirectIoFn writeFn);

  // Forbidden copy constructor and assignment operator
  QsfpUtil(QsfpUtil const&) = delete;
  QsfpUtil& operator=(QsfpUtil const&) = delete;

 private:
  bool directAccess_{false};
  ReadViaServiceFn readViaServiceFn_;
  WriteViaServiceFn writeViaServiceFn_;
  ReadViaDirectIoFn readViaDirectIoFn_;
  WriteViaDirectIoFn writeViaDirectIoFn_;
  folly::EventBase* evb_{nullptr};
};

} // namespace facebook::fboss
