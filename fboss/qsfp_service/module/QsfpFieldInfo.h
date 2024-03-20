// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <unordered_map>
#include "fboss/agent/FbossError.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

namespace facebook {
namespace fboss {

template <typename QsfpField, typename Pages>
class QsfpFieldInfo {
 public:
  uint8_t transceiverI2CAddress;
  int dataAddress; // TODO: rename to page
  int offset;
  int length;

  QsfpFieldInfo(
      uint8_t transceiverI2CAddress,
      Pages page,
      int offset,
      int length)
      : transceiverI2CAddress(transceiverI2CAddress),
        dataAddress(static_cast<int>(page)),
        offset(offset),
        length(length) {}

  QsfpFieldInfo(Pages page, int offset, int length)
      : transceiverI2CAddress(TransceiverAccessParameter::ADDR_QSFP),
        dataAddress(static_cast<int>(page)),
        offset(offset),
        length(length) {}

  typedef std::unordered_map<QsfpField, QsfpFieldInfo> QsfpFieldMap;

  static QsfpFieldInfo getQsfpFieldAddress(
      const QsfpFieldMap& map,
      QsfpField field) {
    auto info = map.find(field);
    if (info == map.end()) {
      throw FbossError("Invalid Field ID");
    }
    return info->second;
  }
};

} // namespace fboss
} // namespace facebook
