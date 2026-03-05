// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"

#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>

#include <chrono>
#include <iostream>

namespace facebook::fboss {

namespace {

// Constants
const folly::IPAddressV6 kDstIp{"2620:0:1cfe:face:b00c::4"};
[[maybe_unused]] constexpr std::chrono::seconds kPacketTimeout{60};
[[maybe_unused]] constexpr uint8_t kNetworkControlDscp = 48;
constexpr size_t kPayloadSize = 12;
const folly::MacAddress kSrcMac{"fa:ce:b0:00:00:0c"};
const folly::IPAddressV6 kSrcIp{"2620:0:1cfe:face:b00c::3"};

// Types
struct DecodedPayload {
  bool valid{false};
  uint64_t timestampNs{0};
  uint32_t sequenceNumber{0};
};

struct LatencyResult {
  bool success{false};
  double latencyMs{0.0};
  uint32_t expectedSeq{0};
  uint32_t receivedSeq{0};
  bool sequenceMatched{false};
};

// Payload encoding/decoding
[[maybe_unused]] std::vector<uint8_t> encodePayload(uint32_t sequenceNumber) {
  folly::IOBuf buf(folly::IOBuf::CREATE, kPayloadSize);
  buf.append(kPayloadSize);

  folly::io::RWPrivateCursor cursor(&buf);
  uint64_t timestampNs =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count();

  cursor.writeBE(timestampNs);
  cursor.writeBE(sequenceNumber);

  return std::vector(buf.data(), buf.data() + buf.length());
}

[[maybe_unused]] DecodedPayload decodePayload(const folly::IOBuf& rxBuf) {
  DecodedPayload result;

  size_t kMinUdpPacketSize = EthHdr::UNTAGGED_PKT_SIZE + IPv6Hdr::SIZE +
      UDPHeader::size() + kPayloadSize;
  size_t totalLength = rxBuf.computeChainDataLength();

  if (totalLength < kMinUdpPacketSize) {
    XLOG(DBG4) << "Packet too small: " << totalLength
               << " bytes, minimum required: " << kMinUdpPacketSize;
    return result;
  }

  folly::io::Cursor cursor(&rxBuf);

  size_t payloadOffset = totalLength - kPayloadSize;
  cursor.skip(payloadOffset);
  result.timestampNs = cursor.readBE<uint64_t>();
  result.sequenceNumber = cursor.readBE<uint32_t>();
  result.valid = true;

  return result;
}

// Packet creation
[[maybe_unused]] std::unique_ptr<TxPacket> createLatencyTestPacket(
    AgentEnsemble* ensemble,
    uint32_t sequenceNumber) {
  auto vlanId = ensemble->getVlanIDForTx();
  auto intfMac =
      utility::getMacForFirstInterfaceWithPorts(ensemble->getProgrammedState());

  auto payload = encodePayload(sequenceNumber);
  uint8_t trafficClass = kNetworkControlDscp << 2;

  return utility::makeUDPTxPacket(
      ensemble->getSw(),
      vlanId,
      kSrcMac,
      intfMac,
      kSrcIp,
      kDstIp,
      8000,
      8001,
      trafficClass,
      255,
      payload);
}

} // namespace
} // namespace facebook::fboss
