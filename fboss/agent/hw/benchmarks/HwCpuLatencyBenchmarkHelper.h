// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

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
constexpr uint16_t kTestSrcPort = 8000;
constexpr uint16_t kTestDstPort = 8001;

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

  cursor.writeBE(timestampNs);
  cursor.writeBE(sequenceNumber);

  return std::vector(buf.data(), buf.data() + buf.length());
}

inline DecodedPayload decodePayload(const folly::IOBuf& rxBuf) {
  DecodedPayload result;

  size_t kMinUdpPacketSize = static_cast<size_t>(EthHdr::UNTAGGED_PKT_SIZE) +
      static_cast<size_t>(IPv6Hdr::SIZE) + UDPHeader::size() + kPayloadSize;
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

// ===== Packet Creation =====

inline std::unique_ptr<TxPacket> createLatencyTestPacket(
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

  // Add trap ACL after ensemble creation when programmed state is available
  auto config = ensemble->getCurrentConfig();
  utility::addTrapPacketAcl(&config, dstMac);
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
      {RoutePrefixV6{folly::IPAddressV6(), 0}});

  utility::ttlDecrementHandlingForLoopbackTraffic(
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

  for (int batch = 0; batch < numBatches; ++batch) {
    auto txPacket = createLatencyTestPacket(ensemble, batch);
    ensemble->getSw()->sendPacketOutOfPortAsync(std::move(txPacket), portId);
    results.totalSent++;

    auto rxBuf = snooper.waitForPacket(kBatchTimeoutSeconds);
    auto rxTime = std::chrono::steady_clock::now();

    if (!rxBuf) {
      results.totalDropped++;
      XLOG(WARNING) << "Packet seq=" << batch << " dropped (timeout)";
      continue;
    }

    auto decoded = decodePayload(**rxBuf);
    CHECK(decoded.valid) << "Invalid payload in received packet";
    results.totalReceived++;

    uint64_t rxTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            rxTime.time_since_epoch())
                            .count();
    double packetLatencyMs =
        static_cast<double>(rxTimeNs - decoded.timestampNs) / 1'000'000.0;
    results.perBatchLatenciesMs.push_back(packetLatencyMs);

    XLOG(INFO) << "Packet seq=" << batch << " latency=" << packetLatencyMs
               << "ms";

    // Sleep between iterations to ensure the rxThread and snooper have fully
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

    folly::dynamic latencyJson = folly::dynamic::object;
    latencyJson["min_ms"] = results.minMs;
    latencyJson["max_ms"] = results.maxMs;
    latencyJson["avg_ms"] = results.avgMs;
    latencyJson["p50_ms"] = results.p50Ms;
    latencyJson["p99_ms"] = results.p99Ms;
    latencyJson["ci_95_lower_ms"] = results.ciLowerMs;
    latencyJson["ci_95_upper_ms"] = results.ciUpperMs;
    json["latency_ms"] = latencyJson;

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
