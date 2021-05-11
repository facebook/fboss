// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/packet/MPLSHdr.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"

#include <folly/io/Cursor.h>

namespace facebook::fboss {

class RxPacket;
class SwSwitch;

class MPLSHandler {
 public:
  explicit MPLSHandler(SwSwitch* sw);
  void handlePacket(
      std::unique_ptr<RxPacket> pkt,
      const MPLSHdr& header,
      folly::io::Cursor cursor);

 private:
  void handleKnownLabel(
      std::unique_ptr<RxPacket> pkt,
      const MPLSHdr& header,
      folly::io::Cursor cursor);

  void handleUnknownLabel(
      std::unique_ptr<RxPacket> pkt,
      const MPLSHdr& header,
      folly::io::Cursor cursor);

  void handleLabel2Me(
      std::unique_ptr<RxPacket> pkt,
      const MPLSHdr& header,
      folly::io::Cursor cursor) const;

  bool isLabelProgrammed(const MPLSHdr& header) const;

  SwSwitch* sw_;
};

} // namespace facebook::fboss
