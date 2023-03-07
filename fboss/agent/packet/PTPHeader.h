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

#include <folly/io/Cursor.h>
#include <cstdint>
#include "fboss/agent/Utils.h"
#include "fboss/agent/packet/HdrParseError.h"
#include "fboss/agent/types.h"

namespace folly {
namespace io {
class Cursor;
}
} // namespace folly

constexpr int PTP_DELAY_REQUEST_MSG_SIZE = 44;
constexpr int PTP_UDP_EVENT_PORT = 319;

namespace facebook::fboss {

enum class PTPMessageType : uint8_t {
  PTP_SYNC = 0x0,
  PTP_DELAY_REQUEST = 0x1,
  PTP_DELAY_RESPONSE = 0x9,
  PTP_UNKNOWN = 0x10,
};

enum class PTPVersion : uint8_t {
  PTP_V1 = 0x1,
  PTP_V2 = 0x2,
};

struct PTPHeader {
 private:
  void parse(folly::io::Cursor* cursor);
  void checkPtpVersionValid();

 public:
  PTPHeader(
      uint8_t ptpType,
      uint8_t ptpVersion,
      uint64_t ptpCorrectionField = 0)
      : ptpType_(ptpType),
        ptpVersion_(ptpVersion),
        ptpCorrectionField_(ptpCorrectionField) {}

  static int getPayloadSize(PTPMessageType ptpPktType);
  void write(folly::io::RWPrivateCursor* cursor) const;

  explicit PTPHeader(folly::io::Cursor* cursor);
  uint64_t getCorrectionField();
  PTPMessageType getPtpType();
  PTPVersion getPtpVersion();
  uint16_t getPtpMessageLength();

  uint8_t ptpType_{static_cast<uint8_t>(
      PTPMessageType::PTP_UNKNOWN)}; // messageType: upper 4 bits only
  uint8_t ptpVersion_{0}; // versionPTP
  uint64_t ptpCorrectionField_{0}; // correction field
  uint16_t ptpMessageLength_; // message length
};

} // namespace facebook::fboss
