// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"

#include <boost/container/flat_set.hpp>
#include <folly/Benchmark.h>
#include <folly/Conv.h>
#include <folly/IPAddress.h>
#include <folly/io/Cursor.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
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
  std::optional<folly::IPAddressV6> dstIp;
};

struct BatchLatencyResults {
  // Counts
  int totalSent{0};
  int totalReceived{0};
  int totalDropped{0};

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
inline DecodedPayload decodePayload(const folly::IOBuf& rxBuf) {
  DecodedPayload result;

  folly::io::Cursor cursor(&rxBuf);
  size_t totalLength = rxBuf.computeChainDataLength();

  if (totalLength < kMinTestPacketSize) {
    XLOG(DBG3) << "Packet too small: " << totalLength << " bytes";
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
    XLOG(DBG3) << "etherType=" << etherType << " != expected IPv6";
    return result;
  }

  // Extract dst IP from IPv6 header (src addr at offset 8, dst addr at offset
  // 24) IPv6 layout: version+tc+fl(4) + payload_len(2) + next_hdr(1) +
  // hop_limit(1)
  //              + src_addr(16) + dst_addr(16) = 40 bytes
  cursor.skip(8); // skip to src addr
  cursor.skip(16); // skip src addr to reach dst addr
  std::array<uint8_t, 16> dstAddrBytes{};
  cursor.pull(dstAddrBytes.data(), 16);
  result.dstIp =
      folly::IPAddressV6::fromBinary(folly::ByteRange(dstAddrBytes.data(), 16));

  // Read UDP src port, dst port (first 4 bytes of UDP header)
  auto udpSrcPort = cursor.readBE<uint16_t>();
  auto udpDstPort = cursor.readBE<uint16_t>();
  (void)udpSrcPort;

  if (udpDstPort != kTestDstPort) {
    XLOG(DBG3) << "udpDstPort=" << udpDstPort << " != expected "
               << kTestDstPort;
    return result;
  }

  // Skip UDP length(2) + checksum(2) to reach payload
  cursor.skip(kUdpHdrSize - 4);

  // Read our payload: timestamp(8) + sequence(4)
  if (cursor.totalLength() < kPayloadSize) {
    XLOG(DBG3) << "Packet too small for cursor be placed in the right place: "
               << cursor.totalLength() << " bytes";
    return result;
  }
  result.timestampNs = cursor.readBE<uint64_t>();
  result.sequenceNumber = cursor.readBE<uint32_t>();
  result.valid = true;

  return result;
}

// ===== Packet Wait Helper =====

struct ValidPacketResult {
  bool received{false};
  DecodedPayload decoded;
  std::chrono::steady_clock::time_point rxTime;
};

// Waits for a valid test packet until the given deadline, skipping
// non-test packets (LLDP, NDP, etc.). Returns received=false on timeout.
inline ValidPacketResult waitForValidPacketUntil(
    utility::SwSwitchPacketSnooper& snooper,
    std::chrono::steady_clock::time_point deadline,
    folly::StringPiece context) {
  while (true) {
    auto remaining = std::chrono::duration_cast<std::chrono::seconds>(
        deadline - std::chrono::steady_clock::now());
    if (remaining.count() <= 0) {
      return {};
    }
    auto rxBuf =
        snooper.waitForPacket(static_cast<uint32_t>(remaining.count()));
    if (!rxBuf) {
      return {};
    }
    auto decoded = decodePayload(**rxBuf);
    if (!decoded.valid) {
      XLOG(DBG3) << context << " skipping non-test packet";
      continue;
    }
    return {true, decoded, std::chrono::steady_clock::now()};
  }
}

// Waits up to kBatchTimeoutSeconds for a valid test packet.
inline ValidPacketResult waitForValidPacket(
    utility::SwSwitchPacketSnooper& snooper,
    folly::StringPiece context) {
  return waitForValidPacketUntil(
      snooper,
      std::chrono::steady_clock::now() +
          std::chrono::seconds(kBatchTimeoutSeconds),
      context);
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
  auto intfMac = getMacForFirstInterfaceWithPortsForTesting(
      ensemble->getProgrammedState());

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

  // Add DstIP-based trap ACL with TRAP action (DstMac not supported on
  // Gibraltar/Graphene200).
  auto config = ensemble->getCurrentConfig();
  auto asic = checkSameAndGetAsicForTesting(ensemble->getL3Asics());
  utility::addTrapPacketAcl(
      asic, &config, folly::CIDRNetwork(kDstIp, 128), cfg::ToCpuAction::TRAP);

  ensemble->applyNewConfig(config);

  XLOG(INFO) << "Loopback traffic path setup complete for port " << portId;

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

    auto rx =
        waitForValidPacket(snooper, folly::to<std::string>("seq=", batch));
    if (!rx.received) {
      results.totalDropped++;
      XLOG(WARNING) << "Packet seq=" << batch << " dropped (timeout)";
      continue;
    }

    results.totalReceived++;

    uint64_t rxTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            rx.rxTime.time_since_epoch())
                            .count();
    double packetLatencyMs =
        static_cast<double>(rxTimeNs - rx.decoded.timestampNs) / 1'000'000.0;
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
               << " Dropped=" << results.totalDropped;
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

  reportResults(results, numBatches, batchSize);
}

// =============================================================================
// All-Port CPU Latency Benchmark
// =============================================================================

struct MultiPortCpuLatencySetup {
  std::unique_ptr<AgentEnsemble> ensemble;
  std::vector<PortID> ports;
  std::map<PortID, folly::IPAddressV6> portToIp;
  std::map<PortID, VlanID> portToVlan;
};

struct MultiPortLatencyResults {
  std::map<PortID, BatchLatencyResults> perPort;
  BatchLatencyResults aggregate;
};

// ===== All-Port Ensemble Setup =====
//
// Configures all interface ports with loopback routes.
// Pattern inlined from setupEcmpDataplaneLoopOnAllPorts() to avoid
// gtest dependency (MultiPortTrafficTestUtils.h includes AgentHwTest.h).

inline MultiPortCpuLatencySetup createAllPortCpuLatencyEnsemble() {
  AgentEnsembleSwitchConfigFn initialConfigFn =
      [](const AgentEnsemble& ensemble) -> cfg::SwitchConfig {
    auto allPorts = ensemble.masterLogicalInterfacePortIds();
    CHECK_GE(allPorts.size(), 1);

    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(), allPorts, true /*interfaceHasSubnet*/);

    utility::addOlympicQosMaps(config, ensemble.getL3Asics());
    utility::setDefaultCpuTrafficPolicyConfig(
        config, ensemble.getL3Asics(), ensemble.isSai());
    utility::addCpuQueueConfig(config, ensemble.getL3Asics(), ensemble.isSai());

    return config;
  };

  auto ensemble =
      createAgentEnsemble(initialConfigFn, false /*disableLinkStateToggler*/);

  auto allPorts = ensemble->masterLogicalInterfacePortIds();
  auto intfMac = getMacForFirstInterfaceWithPortsForTesting(
      ensemble->getProgrammedState());

  // Per-port IP and VLAN mappings
  // onePortPerInterfaceConfig assigns VlanID(kBaseVlanId + index) per port
  constexpr int kBaseVlanId = 2000;
  std::map<PortID, folly::IPAddressV6> portToIp;
  std::map<PortID, VlanID> portToVlan;
  for (int idx = 0; idx < allPorts.size(); idx++) {
    portToIp[allPorts[idx]] =
        folly::IPAddressV6(folly::to<std::string>("2401::", idx + 1));
    portToVlan[allPorts[idx]] = VlanID(kBaseVlanId + idx);
  }

  // Per-port /128 TRAP ACL for each port's IP.
  // addTrapPacketAcl discards prefix length, so /64 doesn't work.
  auto config = ensemble->getCurrentConfig();
  auto asic = checkSameAndGetAsicForTesting(ensemble->getL3Asics());
  for (auto& [portId, ip] : portToIp) {
    utility::addTrapPacketAcl(
        asic, &config, folly::CIDRNetwork(ip, 128), cfg::ToCpuAction::TRAP);
  }
  ensemble->applyNewConfig(config);

  // Setup ECMP loopback routes (inlined from setupEcmpDataplaneLoopOnAllPorts)
  utility::EcmpSetupTargetedPorts6 ecmpHelper(
      ensemble->getProgrammedState(),
      ensemble->getSw()->needL2EntryForNeighbor(),
      intfMac);

  std::vector<PortDescriptor> portDescriptors;
  std::vector<boost::container::flat_set<PortDescriptor>> portDescSets;
  for (auto& portId : allPorts) {
    portDescriptors.emplace_back(portId);
    portDescSets.push_back(
        boost::container::flat_set<PortDescriptor>{PortDescriptor(portId)});
  }

  ensemble->applyNewState(
      [&portDescriptors, &ecmpHelper](const std::shared_ptr<SwitchState>& in) {
        return ecmpHelper.resolveNextHops(
            in,
            boost::container::flat_set<PortDescriptor>(
                std::make_move_iterator(portDescriptors.begin()),
                std::make_move_iterator(portDescriptors.end())));
      });

  std::vector<RoutePrefixV6> routePrefixes;
  routePrefixes.reserve(portToIp.size());
  for (auto& [portId, ip] : portToIp) {
    routePrefixes.emplace_back(ip, 128);
  }
  auto routeUpdater = ensemble->getSw()->getRouteUpdater();
  ecmpHelper.programRoutes(&routeUpdater, portDescSets, routePrefixes);

  for (auto& nhop : ecmpHelper.getNextHops()) {
    utility::disablePortTTLDecrementIfSupported(
        ensemble.get(), ecmpHelper.getRouterId(), nhop);
  }

  XLOG(INFO) << "All-port loopback setup complete for " << allPorts.size()
             << " ports";

  return {std::move(ensemble), allPorts, portToIp, portToVlan};
}

// ===== Sequential All-Port Measurement =====

inline MultiPortLatencyResults measureCpuLatencySequentialAllPorts(
    AgentEnsemble* ensemble,
    const std::vector<PortID>& ports,
    const std::map<PortID, folly::IPAddressV6>& portToIp,
    const std::map<PortID, VlanID>& portToVlan,
    int numBatches) {
  MultiPortLatencyResults results;

  utility::SwSwitchPacketSnooper snooper(
      ensemble->getSw(), "cpuLatencyAllPortsBenchmark");
  snooper.ignoreUnclaimedRxPkts();

  for (auto portId : ports) {
    BatchLatencyResults portResults;
    auto dstIp = portToIp.at(portId);
    auto vlanId = portToVlan.at(portId);

    for (int batch = 0; batch < numBatches; ++batch) {
      auto txPacket = createLatencyTestPacket(ensemble, batch, dstIp, vlanId);
      ensemble->getSw()->sendPacketOutOfPortAsync(std::move(txPacket), portId);
      portResults.totalSent++;

      auto rx = waitForValidPacket(
          snooper, folly::to<std::string>("Port ", portId, " seq=", batch));
      if (!rx.received) {
        portResults.totalDropped++;
        XLOG(WARNING) << "Port " << portId << " seq=" << batch
                      << " dropped (timeout)";
        continue;
      }

      portResults.totalReceived++;

      uint64_t rxTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              rx.rxTime.time_since_epoch())
                              .count();
      double packetLatencyMs =
          static_cast<double>(rxTimeNs - rx.decoded.timestampNs) / 1'000'000.0;
      portResults.perBatchLatenciesMs.push_back(packetLatencyMs);

      XLOG(INFO) << "Port " << portId << " seq=" << batch
                 << " latency=" << packetLatencyMs << "ms";

      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    computeStats(portResults);
    results.perPort[portId] = portResults;

    for (auto latency : portResults.perBatchLatenciesMs) {
      results.aggregate.perBatchLatenciesMs.push_back(latency);
    }
    results.aggregate.totalSent += portResults.totalSent;
    results.aggregate.totalReceived += portResults.totalReceived;
    results.aggregate.totalDropped += portResults.totalDropped;
  }

  computeStats(results.aggregate);
  return results;
}

// ===== Multi-Port Results Reporting =====

inline void reportMultiPortResults(
    const MultiPortLatencyResults& results,
    int numBatches,
    const std::string& mode) {
  // Per-port results: avg latency and timestamp pairs
  for (auto& [portId, portResults] : results.perPort) {
    XLOG(INFO) << "Port " << portId << ": sent=" << portResults.totalSent;
    XLOG(INFO) << "Port " << portId << ": avg=" << portResults.avgMs
               << "ms p99=" << portResults.p99Ms << "ms"
               << " sent=" << portResults.totalSent
               << " received=" << portResults.totalReceived
               << " dropped=" << portResults.totalDropped;
  }

  auto& agg = results.aggregate;
  if (FLAGS_json) {
    // Flat JSON with per-port averages (no nested objects for netcastle)
    folly::dynamic json = folly::dynamic::object;
    json["mode"] = mode;
    json["num_batches"] = numBatches;
    json["num_ports"] = static_cast<int>(results.perPort.size());
    json["total_sent"] = agg.totalSent;
    json["total_received"] = agg.totalReceived;
    json["total_dropped"] = agg.totalDropped;

    // Per-port average latency as flat keys: port_<id>_avg_ms
    for (auto& [portId, portResults] : results.perPort) {
      auto key =
          folly::to<std::string>("port_", static_cast<int>(portId), "_avg_ms");
      json[key] = portResults.avgMs;
    }

    // Aggregate stats
    json["latency_min_ms"] = agg.minMs;
    json["latency_max_ms"] = agg.maxMs;
    json["latency_avg_ms"] = agg.avgMs;
    json["latency_p50_ms"] = agg.p50Ms;
    json["latency_p99_ms"] = agg.p99Ms;
    json["latency_ci_95_lower_ms"] = agg.ciLowerMs;
    json["latency_ci_95_upper_ms"] = agg.ciUpperMs;

    std::cout << toPrettyJson(json) << std::endl;
  } else {
    XLOG(INFO) << "=== All-Port CPU Latency (" << mode << ") ===";
    XLOG(INFO) << "Ports=" << results.perPort.size()
               << " Batches=" << numBatches;
    XLOG(INFO) << "Aggregate: Sent=" << agg.totalSent
               << " Received=" << agg.totalReceived
               << " Dropped=" << agg.totalDropped;
    XLOG(INFO) << "Latency(ms): min=" << agg.minMs << " max=" << agg.maxMs
               << " avg=" << agg.avgMs << " p50=" << agg.p50Ms
               << " p99=" << agg.p99Ms;
    XLOG(INFO) << "95% CI: [" << agg.ciLowerMs << ", " << agg.ciUpperMs << "]";
  }
}

// ===== Sequential All-Port Benchmark Function =====

inline void cpuLatencySequentialAllPortsBenchmark(
    int numBatches,
    int /*batchSize*/) {
  folly::BenchmarkSuspender suspender;
  auto setup = createAllPortCpuLatencyEnsemble();
  suspender.dismiss();

  auto results = measureCpuLatencySequentialAllPorts(
      setup.ensemble.get(),
      setup.ports,
      setup.portToIp,
      setup.portToVlan,
      numBatches);

  suspender.rehire();

  CHECK_GT(results.aggregate.totalReceived, 0) << "No packets received";
  CHECK_EQ(results.aggregate.totalDropped, 0)
      << "Packets dropped in lab environment";

  reportMultiPortResults(results, numBatches, "sequential");
}

// ===== Concurrent All-Port Measurement =====
//
// Each round: send one packet on every port, then collect all responses
// via waitForAnyPacket(), attribute by returned portId.

inline MultiPortLatencyResults measureCpuLatencyConcurrentAllPorts(
    AgentEnsemble* ensemble,
    const std::vector<PortID>& ports,
    const std::map<PortID, folly::IPAddressV6>& portToIp,
    const std::map<PortID, VlanID>& portToVlan,
    int numBatches) {
  MultiPortLatencyResults results;

  // Initialize per-port results
  for (const auto& portId : ports) {
    results.perPort[portId] = BatchLatencyResults{};
  }

  // Build reverse map: IP -> PortID for port attribution
  std::map<folly::IPAddressV6, PortID> ipToPort;
  for (auto& [portId, ip] : portToIp) {
    ipToPort[ip] = portId;
  }

  utility::SwSwitchPacketSnooper snooper(
      ensemble->getSw(), "cpuLatencyConcurrentBenchmark");
  snooper.ignoreUnclaimedRxPkts();

  for (int batch = 0; batch < numBatches; ++batch) {
    // Send one packet on every port
    XLOG(INFO) << "Concurrent batch " << batch << " sending on " << ports.size()
               << " ports";
    for (const auto& portId : ports) {
      auto dstIp = portToIp.at(portId);
      auto vlanId = portToVlan.at(portId);
      auto txPacket = createLatencyTestPacket(ensemble, batch, dstIp, vlanId);
      ensemble->getSw()->sendPacketOutOfPortAsync(std::move(txPacket), portId);
      results.perPort[portId].totalSent++;
    }

    // Collect all responses, skipping non-test packets (LLDP, NDP, etc.)
    int collected = 0;
    int expectedResponses = static_cast<int>(ports.size());
    auto deadline = std::chrono::steady_clock::now() +
        std::chrono::seconds(kBatchTimeoutSeconds);
    while (collected < expectedResponses) {
      auto rx = waitForValidPacketUntil(
          snooper,
          deadline,
          folly::to<std::string>("Concurrent batch=", batch));
      if (!rx.received) {
        break;
      }

      collected++;
      results.aggregate.totalReceived++;
      uint64_t rxTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                              rx.rxTime.time_since_epoch())
                              .count();
      double packetLatencyMs =
          static_cast<double>(rxTimeNs - rx.decoded.timestampNs) / 1'000'000.0;
      results.aggregate.perBatchLatenciesMs.push_back(packetLatencyMs);

      // Attribute to port via dstIp
      if (rx.decoded.dstIp) {
        auto portIt = ipToPort.find(*rx.decoded.dstIp);
        if (portIt != ipToPort.end()) {
          auto& portResults = results.perPort[portIt->second];
          portResults.totalReceived++;
          portResults.perBatchLatenciesMs.push_back(packetLatencyMs);
          XLOG(INFO) << "Concurrent batch=" << batch
                     << " port=" << portIt->second
                     << " latency=" << packetLatencyMs << "ms";
        } else {
          XLOG(WARNING) << "Concurrent batch=" << batch
                        << " unknown dstIp in received packet";
        }
      }
    }

    // Check for drops this round
    int sentThisRound = static_cast<int>(ports.size());
    int droppedThisRound = sentThisRound - collected;
    if (droppedThisRound > 0) {
      results.aggregate.totalDropped += droppedThisRound;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  // Compute aggregate and per-port stats
  results.aggregate.totalSent = numBatches * static_cast<int>(ports.size());
  computeStats(results.aggregate);
  for (auto& [portId, portResults] : results.perPort) {
    portResults.totalDropped =
        portResults.totalSent - portResults.totalReceived;
    XLOG(INFO) << "Port " << portId << " sent=" << portResults.totalSent
               << " received=" << portResults.totalReceived
               << " dropped=" << portResults.totalDropped;
    computeStats(portResults);
  }

  return results;
}

// ===== Concurrent All-Port Benchmark Function =====

inline void cpuLatencyConcurrentAllPortsBenchmark(
    int numBatches,
    int /*batchSize*/) {
  folly::BenchmarkSuspender suspender;
  auto setup = createAllPortCpuLatencyEnsemble();
  suspender.dismiss();

  auto results = measureCpuLatencyConcurrentAllPorts(
      setup.ensemble.get(),
      setup.ports,
      setup.portToIp,
      setup.portToVlan,
      numBatches);

  suspender.rehire();

  CHECK_GT(results.aggregate.totalReceived, 0) << "No packets received";

  if (results.aggregate.totalDropped > 0) {
    XLOG(WARNING) << "Concurrent benchmark: " << results.aggregate.totalDropped
                  << "/" << results.aggregate.totalSent << " packets dropped";
  }

  reportMultiPortResults(results, numBatches, "concurrent");
}

} // namespace

// ===== Benchmark Macros =====

#define CPU_LATENCY_BENCHMARK(name, numBatches, batchSize) \
  BENCHMARK(name) {                                        \
    cpuLatencyBenchmark(numBatches, batchSize);            \
  }

#define CPU_LATENCY_SEQ_ALL_PORTS_BENCHMARK(name, numBatches, batchSize) \
  BENCHMARK(name) {                                                      \
    cpuLatencySequentialAllPortsBenchmark(numBatches, batchSize);        \
  }

#define CPU_LATENCY_CONCURRENT_ALL_PORTS_BENCHMARK(               \
    name, numBatches, batchSize)                                  \
  BENCHMARK(name) {                                               \
    cpuLatencyConcurrentAllPortsBenchmark(numBatches, batchSize); \
  }

} // namespace facebook::fboss
