// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>

#include <folly/io/Cursor.h>

namespace facebook::fboss {

enum class TajoPuntNextHeaderKind {
  Unknown,
  Ethernet,
  Ipv4,
  Ipv6,
};

constexpr size_t kTajoModOuterEthLen = 14;
constexpr size_t kTajoModOuterIpv6Len = 40;
constexpr size_t kTajoModOuterUdpLen = 8;
constexpr size_t kTajoNplPuntHeaderBytes = 40;
constexpr size_t kTajoModInnerEthLen = 14;
constexpr size_t kTajoModEneDummyHdrLen = 18;

// Bytes of mirrored sample after the punt header on the wire.
inline size_t tajoModInnerBytesOnWire(
    size_t frameLen,
    size_t outerEthLen,
    size_t puntConsumedBytes = kTajoNplPuntHeaderBytes) {
  if (frameLen < outerEthLen + kTajoModOuterIpv6Len + kTajoModOuterUdpLen +
          puntConsumedBytes) {
    return 0;
  }
  return frameLen - outerEthLen - kTajoModOuterIpv6Len - kTajoModOuterUdpLen -
      puntConsumedBytes;
}

struct ParsedTajoPuntHeader {
  uint8_t nextHeader{0};
  uint8_t fwdHeaderType{0};
  uint8_t tc{0};
  uint8_t nextHeaderOffset{0};
  uint8_t puntSource{0};
  uint8_t code{0};
  uint8_t lptsFlowType{0};
  uint16_t sourceSp{0};
  uint16_t destinationSp{0};
  uint32_t sourceLp{0};
  uint32_t destinationLp{0};
  uint16_t relayId{0};
  uint64_t timeStamp{0};
  uint32_t receiveTime{0};
  uint8_t version{0};
  uint8_t fwdQosTag{0};
  uint8_t qosGroup{0};
  uint8_t encapQosTag{0};

  uint16_t ingressPort{0};
  uint16_t dropReason{0};
  TajoPuntNextHeaderKind nextHeaderKind{TajoPuntNextHeaderKind::Unknown};
  uint32_t consumedBytes{0};
  bool parsedBySdk{false};
  bool parsedByWire{false};
};

inline bool tajoPuntInnerHasEthernetHdr(TajoPuntNextHeaderKind kind) {
  return kind == TajoPuntNextHeaderKind::Ethernet;
}

inline bool tajoPuntInnerStartsAtL3(TajoPuntNextHeaderKind kind) {
  return kind == TajoPuntNextHeaderKind::Ipv4 ||
      kind == TajoPuntNextHeaderKind::Ipv6;
}

ParsedTajoPuntHeader parseTajoPuntHeader(folly::io::Cursor& cursor);

size_t tajoModExpectedOuterIpv6PayloadLen(
    size_t innerBytesOnWire,
    size_t truncateSize = 0);

} // namespace facebook::fboss
