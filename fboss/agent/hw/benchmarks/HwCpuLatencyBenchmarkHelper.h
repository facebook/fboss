// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"

#include <boost/container/flat_set.hpp>
#include <folly/Benchmark.h>
#include <folly/IPAddress.h>
#include <folly/io/Cursor.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

namespace facebook::fboss {

namespace {

// Constants
inline const folly::IPAddressV6 kDstIp{"2620:0:1cfe:face:b00c::4"};
inline const folly::IPAddressV6 kSrcIp{"2620:0:1cfe:face:b00c::3"};
inline const folly::MacAddress kSrcMac{"fa:ce:b0:00:00:0c"};
constexpr uint8_t kNetworkControlDscp = 48;
constexpr size_t kPayloadSize = 12;
constexpr int kBatchTimeoutSeconds = 120;
constexpr uint16_t kTestSrcPort = 49109;
constexpr uint16_t kTestDstPort = 49110;
constexpr uint16_t kEtherTypeIPv6 = 0x86DD;
constexpr uint16_t kEtherTypeVlan = 0x8100;
constexpr uint16_t kEtherTypeQinQ = 0x88A8;
constexpr size_t kEthMacsSize = 12; // dst(6) + src(6)
constexpr size_t kVlanTciSize = 2;
constexpr size_t kIPv6HdrSize = 40;
constexpr size_t kUdpHdrSize = 8;
constexpr size_t kMinTestPacketSize =
    kEthMacsSize + 2 + kIPv6HdrSize + kUdpHdrSize + kPayloadSize; // 74

// ===== Structs =====

struct DecodedPayload {
  bool valid{false};
  uint64_t timestampNs{0};
  uint32_t sequenceNumber{0};
};

struct BatchLatencyResults {
  // Counts
  int totalSent{0};
  int totalReceived{0};
  int totalDropped{0};
  int outOfOrder{0};

  // Per-batch latency: one value per batch (send-start to last-receive)
  std::vector<double> perBatchLatenciesMs;

  // Aggregate stats (computed after all batches)
  double minMs{0.0};
  double maxMs{0.0};
  double avgMs{0.0};
  double p50Ms{0.0};
  double p99Ms{0.0};
  double ciLowerMs{0.0}; // 95% confidence interval lower bound
  double ciUpperMs{0.0}; // 95% confidence interval upper bound
};

struct CpuLatencyBenchmarkSetup {
  std::unique_ptr<AgentEnsemble> ensemble;
  PortID portId;
};

// ===== Payload Encoding / Decoding =====

inline std::vector<uint8_t> encodePayload(uint32_t sequenceNumber) {
  folly::IOBuf buf(folly::IOBuf::CREATE, kPayloadSize);
  buf.append(kPayloadSize);

  folly::io::RWPrivateCursor cursor(&buf);
  uint64_t timestampNs =
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count();

  cursor.writeBE<uint64_t>(timestampNs);
  cursor.writeBE<uint32_t>(sequenceNumber);

  return std::vector(buf.data(), buf.data() + buf.length());
}

// Parse headers from the beginning to find the UDP payload.
// Validates: EtherType is IPv6, UDP dst port matches kTestDstPort.
// Returns valid=false for non-test packets (LLDP, NDP, etc.).
// Parse headers from the beginning to find the UDP payload.
// Validates: EtherType is IPv6, UDP dst port matches kTestDstPort.
// Returns valid=false for non-test packets (LLDP, NDP, etc.).
inline DecodedPayload decodePayload(const folly::IOBuf& rxBuf) {
  DecodedPayload result;

  folly::io::Cursor cursor(&rxBuf);
  size_t totalLength = rxBuf.computeChainDataLength();

  if (totalLength < kMinTestPacketSize) {
    return result;
  }

  // Parse Ethernet header: skip dst + src MAC, read EtherType
  cursor.skip(kEthMacsSize);
  uint16_t etherType = cursor.readBE<uint16_t>();

  // Skip VLAN tags if present
  while (etherType == kEtherTypeVlan || etherType == kEtherTypeQinQ) {
    cursor.skip(kVlanTciSize);
    etherType = cursor.readBE<uint16_t>();
  }

  if (etherType != kEtherTypeIPv6) {
    return result;
  }

  // Skip IPv6 header
  cursor.skip(kIPv6HdrSize);

  // Read UDP src port, dst port (first 4 bytes of UDP header)
  auto udpSrcPort = cursor.readBE<uint16_t>();
  auto udpDstPort = cursor.readBE<uint16_t>();
  (void)udpSrcPort;

  if (udpDstPort != kTestDstPort) {
    return result;
  }

  // Skip UDP length(2) + checksum(2) to reach payload
  cursor.skip(kUdpHdrSize - 4);

  // Read our payload: timestamp(8) + sequence(4)
  if (cursor.totalLength() < kPayloadSize) {
    return result;
  }
  result.timestampNs = cursor.readBE<uint64_t>();
  result.sequenceNumber = cursor.readBE<uint32_t>();
  result.valid = true;

  return result;
}

// ===== Packet Creation =====

inline std::unique_ptr<TxPacket> createLatencyTestPacket(
    AgentEnsemble* ensemble,
    uint32_t sequenceNumber,
    const folly::IPAddressV6& dstIp = kDstIp,
    std::optional<VlanID> vlanId = std::nullopt) {
  if (!vlanId) {
    vlanId = ensemble->getVlanIDForTx();
  }
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
      dstIp,
      kTestSrcPort,
      kTestDstPort,
      trafficClass,
      255,
      payload);
}

// ===== Ensemble Setup =====

inline CpuLatencyBenchmarkSetup createCpuLatencyEnsemble() {
  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](const AgentEnsemble& ensemble) -> cfg::SwitchConfig {
    CHECK_GE(
        ensemble.masterLogicalPortIds({cfg::PortType::INTERFACE_PORT}).size(),
        1);
    std::vector<PortID> ports = {ensemble.masterLogicalInterfacePortIds()[0]};

    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(), ports, true /*interfaceHasSubnet*/);

    utility::addOlympicQosMaps(config, ensemble.getL3Asics());
    utility::setDefaultCpuTrafficPolicyConfig(
        config, ensemble.getL3Asics(), ensemble.isSai());
    utility::addCpuQueueConfig(config, ensemble.getL3Asics(), ensemble.isSai());

    return config;
  };

  auto ensemble =
      createAgentEnsemble(initialConfigFn, false /*disableLinkStateToggler*/);

  auto portId = ensemble->masterLogicalInterfacePortIds()[0];
  auto dstMac =
      utility::getMacForFirstInterfaceWithPorts(ensemble->getProgrammedState());

  // Add DstIP-based trap ACL with TRAP action (DstMac not supported on
  // Gibraltar/Graphene200).
  auto config = ensemble->getCurrentConfig();
  auto asic = checkSameAndGetAsic(ensemble->getL3Asics());
  utility::addTrapPacketAcl(
      asic, &config, folly::CIDRNetwork(kDstIp, 128), cfg::ToCpuAction::TRAP);

  ensemble->applyNewConfig(config);

  auto ecmpHelper = utility::EcmpSetupAnyNPorts6(
      ensemble->getProgrammedState(),
      ensemble->getSw()->needL2EntryForNeighbor(),
      dstMac);

  flat_set<PortDescriptor> portSet{PortDescriptor(portId)};

  ensemble->applyNewState(
      [&](const std::shared_ptr<SwitchState>& in)
          -> std::shared_ptr<SwitchState> {
        return ecmpHelper.resolveNextHops(in, portSet);
      });

  ecmpHelper.programRoutes(
      std::make_unique<SwSwitchRouteUpdateWrapper>(
          ensemble->getSw(), ensemble->getSw()->getRib()),
      portSet,
      {RoutePrefixV6{folly::IPAddressV6(), 0}},
      {},
      true);
  utility::disablePortTTLDecrementIfSupported(
      ensemble.get(), ecmpHelper.getRouterId(), ecmpHelper.getNextHops()[0]);

  XLOG(DBG2) << "Loopback traffic path setup complete for port " << portId;

  // Log port state for debugging

  return {std::move(ensemble), portId};
}

// ===== Statistics Computation =====

inline void computeStats(BatchLatencyResults& results) {
  auto& all = results.perBatchLatenciesMs;
  if (all.empty()) {
    return;
  }

  size_t n = all.size();

  // Mean and stddev (needed for CI calculation)
  double sum = 0.0;
  for (auto v : all) {
    sum += v;
  }
  double avg = sum / n;

  double sqSum = 0.0;
  for (auto v : all) {
    double diff = v - avg;
    sqSum += diff * diff;
  }
  double stddev = std::sqrt(sqSum / n);

  // Sort for percentiles and min/max
  std::sort(all.begin(), all.end());
  results.minMs = all.front();
  results.maxMs = all.back();
  results.avgMs = avg;
  results.p50Ms = all[static_cast<size_t>(0.50 * (n - 1))];
  results.p99Ms = all[static_cast<size_t>(0.99 * (n - 1))];

  // 95% confidence interval for the mean
  double marginOfError = 1.96 * (stddev / std::sqrt(n));
  results.ciLowerMs = avg - marginOfError;
  results.ciUpperMs = avg + marginOfError;
}

// ===== Core Measurement =====
//
// Send-wait-measure approach: send one packet at a time, wait for it to
// arrive, then measure latency. This avoids all queuing artifacts from
// the snooper queue and rxThread sleep cycles.
//
// Each iteration:
//   SendPacket(seq) → waitForPacket() → measureTime()

inline BatchLatencyResults measureCpuLatency(
    AgentEnsemble* ensemble,
    PortID portId,
    int numBatches,
    int /*batchSize*/) {
  BatchLatencyResults results;

  utility::SwSwitchPacketSnooper snooper(
      ensemble->getSw(), "cpuLatencyBenchmark");
  snooper.ignoreUnclaimedRxPkts();

  for (int batch = 0; batch < numBatches; ++batch) {
    auto txPacket = createLatencyTestPacket(ensemble, batch);
    ensemble->getSw()->sendPacketOutOfPortAsync(std::move(txPacket), portId);
    results.totalSent++;

    // Wait for a valid test packet, skipping non-test packets (LLDP, NDP)
    auto deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(kBatchTimeoutSeconds);
    std::chrono::steady_clock::time_point rxTime;
    DecodedPayload decoded;
    bool gotValidPacket = false;

    while (!gotValidPacket) {
      auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
          deadline - std::chrono::steady_clock::now());
      if (remaining.count() <= 0) {
        break;
      }
      auto rxBuf =
          snooper.waitForPacket(static_cast<uint32_t>(remaining.count()));
      if (!rxBuf) {
        break;
      }
      rxTime = std::chrono::steady_clock::now();
      decoded = decodePayload(**rxBuf);
      if (!decoded.valid) {
        XLOG(DBG2) << "Packet seq=" << batch << " skipping non-test packet";
        continue;
      }
      gotValidPacket = true;
    }

    if (!gotValidPacket) {
      results.totalDropped++;
      XLOG(WARNING) << "Packet seq=" << batch << " dropped (timeout)";
      continue;
    }
    results.totalReceived++;

    uint64_t rxTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            rxTime.time_since_epoch())
                            .count();
    double packetLatencyMs =
        static_cast<double>(rxTimeNs - decoded.timestampNs) / 1'000'000.0;
    results.perBatchLatenciesMs.push_back(packetLatencyMs);

    XLOG(INFO) << "Packet seq=" << batch << " latency=" << packetLatencyMs
               << "ms";

    // Sleep between iterations to ensure the observer has fully
    // settled, preventing overlap with the next packet's measurement.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  computeStats(results);
  return results;
}

// ===== Results Reporting =====

inline void reportResults(
    const BatchLatencyResults& results,
    int numBatches,
    int batchSize) {
  if (FLAGS_json) {
    folly::dynamic json = folly::dynamic::object;
    json["num_batches"] = numBatches;
    json["batch_size"] = batchSize;
    json["total_sent"] = results.totalSent;
    json["total_received"] = results.totalReceived;
    json["total_dropped"] = results.totalDropped;
    json["out_of_order"] = results.outOfOrder;
    json["latency_min_ms"] = results.minMs;
    json["latency_max_ms"] = results.maxMs;
    json["latency_avg_ms"] = results.avgMs;
    json["latency_p50_ms"] = results.p50Ms;
    json["latency_p99_ms"] = results.p99Ms;
    json["latency_ci_95_lower_ms"] = results.ciLowerMs;
    json["latency_ci_95_upper_ms"] = results.ciUpperMs;

    std::cout << toPrettyJson(json) << std::endl;
  } else {
    XLOG(INFO) << "=== CPU Latency Benchmark Results ===";
    XLOG(INFO) << "Config: " << numBatches << " batches x " << batchSize
               << " packets";
    XLOG(INFO) << "Sent=" << results.totalSent
               << " Received=" << results.totalReceived
               << " Dropped=" << results.totalDropped
               << " OutOfOrder=" << results.outOfOrder;
    XLOG(INFO) << "Latency(ms): min=" << results.minMs
               << " max=" << results.maxMs << " avg=" << results.avgMs
               << " p50=" << results.p50Ms << " p99=" << results.p99Ms;
    XLOG(INFO) << "95% CI: [" << results.ciLowerMs << ", " << results.ciUpperMs
               << "]";
  }
}

// ===== Top-Level Benchmark Function =====

inline void cpuLatencyBenchmark(int numBatches, int batchSize) {
  folly::BenchmarkSuspender suspender;
  auto setup = createCpuLatencyEnsemble();
  suspender.dismiss();

  auto results = measureCpuLatency(
      setup.ensemble.get(), setup.portId, numBatches, batchSize);

  suspender.rehire();

  // Correctness checks
  CHECK_GT(results.totalReceived, 0) << "No packets received";
  CHECK_EQ(results.totalDropped, 0) << "Packets dropped in lab environment";
  CHECK_EQ(results.outOfOrder, 0) << "Packets received out of order";

  reportResults(results, numBatches, batchSize);
}

} // namespace

// ===== Benchmark Macro =====

#define CPU_LATENCY_BENCHMARK(name, numBatches, batchSize) \
  BENCHMARK(name) {                                        \
    cpuLatencyBenchmark(numBatches, batchSize);            \
  }

} // namespace facebook::fboss
