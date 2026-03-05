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
constexpr std::chrono::seconds kPacketTimeout{60};
constexpr uint8_t kNetworkControlDscp = 48;
constexpr size_t kPayloadSize = 12;

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
} // namespace
} // namespace facebook::fboss
