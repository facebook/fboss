// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/TestDataFactory.h"
#include <fmt/format.h>
#include <folly/IPAddress.h>

#include "fboss/thrift_cow/nodes/Serializer.h"

// Additional includes for AgentStats
#include "fboss/agent/gen-cpp2/agent_stats_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/lib/if/gen-cpp2/io_stats_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss::test_data {

TaggedOperState TestDataFactory::getStateUpdate(int version, bool minimal) {
  TaggedOperState state;
  OperState chunk;
  std::vector<std::string> basePath;
  chunk.protocol() = protocol_;
  if (minimal) {
    int key = 42;
    auto data = buildMinimalTestData(version, key, basePath);
    chunk.contents() = facebook::fboss::thrift_cow::serialize<
        apache::thrift::type_class::structure>(protocol_, data);
    CHECK_EQ(basePath.size(), 2);
  } else {
    auto data = buildTestData(version, basePath);
    chunk.contents() = facebook::fboss::thrift_cow::serialize<
        apache::thrift::type_class::structure>(protocol_, data);
  }
  state.state() = chunk;
  state.path()->path() = basePath;
  return state;
}

OtherStruct TestDataFactory::buildMinimalTestData(
    int version,
    int key,
    std::vector<std::string>& path) {
  OtherStruct val;

  val.o() = key;
  val.m()[fmt::format("m.key{}", key)] = 900 + version;

  path.emplace_back("mapOfStructs");
  path.emplace_back(fmt::format("key{}", key));

  return val;
}

TestStruct TestDataFactory::buildTestData(
    int version,
    std::vector<std::string>& /* path */) {
  TestStruct val;

  val.tx() = (version % 2) ? false : true;
  val.name() = "str";
  val.optionalString() = "optionalStr";
  val.enumeration() = TestEnum::FIRST;
  val.enumSet() = {TestEnum::FIRST, TestEnum::SECOND};
  val.integralSet() = {101, 102, 103};
  val.listOfPrimitives() = {201, 202, 203};

  auto makeOtherStruct = [](int idx, int mapSize) -> OtherStruct {
    OtherStruct other;
    other.o() = idx;
    for (int i = 0; i < mapSize; i++) {
      other.m()[fmt::format("m.key{}", i)] = 900 + i;
    }
    return other;
  };

  val.listofStructs()->emplace_back(makeOtherStruct(701, 2));
  val.listofStructs()->emplace_back(makeOtherStruct(702, 2));

  if (selector_ != Minimal) {
    int outMapSize = scaleFactor_;
    constexpr int kInnerMapSize = 8;
    for (int i = 0; i < outMapSize; i++) {
      val.mapOfStructs()[fmt::format("key{}", i)] =
          makeOtherStruct(i, kInnerMapSize);
    }
  }

  return val;
}

TaggedOperState FsdbStateDataFactory::getStateUpdate(
    int /* unused */,
    bool /* unused */) {
  TaggedOperState state;
  OperState chunk;
  std::vector<std::string> basePath;
  chunk.protocol() = protocol_;

  auto fsdbRoot = buildFsdbOperStateRoot();

  chunk.contents() = facebook::fboss::thrift_cow::serialize<
      apache::thrift::type_class::structure>(protocol_, fsdbRoot);

  state.state() = std::move(chunk);
  state.path()->path() = std::move(basePath);
  return state;
}

fsdb::FsdbOperStateRoot FsdbStateDataFactory::buildFsdbOperStateRoot() {
  fsdb::FsdbOperStateRoot root;

  fsdb::AgentData agentData;
  agentData.switchState() = buildSwitchState();

  root.agent() = std::move(agentData);
  root.bgp() = fsdb::BgpData{};
  root.openr() = fsdb::OpenrData{};

  return root;
}

SwitchState FsdbStateDataFactory::buildSwitchState() {
  SwitchState switchState;

  auto fibsData = buildFibData();
  std::string switchIdList = "Id:0";
  std::map<int16_t, FibContainerFields> FibsMap;
  FibsMap[0] = std::move(fibsData);
  switchState.fibsMap()[switchIdList] = std::move(FibsMap);

  return switchState;
}

FibContainerFields FsdbStateDataFactory::buildFibData(void) {
  FibContainerFields fibContainer;
  FibScale scale = getRoleScale(selector_);

  fibContainer.vrf() = 0; // Default VRF

  // Generate V4 routes
  for (int i = 0; i < scale.fibV4Size; i++) {
    std::string prefix;
    if (i == 0) {
      prefix = "0.0.0.0/0";
    } else {
      // Generate /24 routes in 10.0.0.0/8 space
      int octet2 = (i / 256) % 256;
      int octet3 = i % 256;
      prefix = fmt::format("10.{}.{}.0/24", octet2, octet3);
    }

    auto nexthops = createNextHops(scale.v4Nexthops, false);
    auto routeFields = createRouteFields(prefix, nexthops);
    fibContainer.fibV4()[prefix] = std::move(routeFields);
  }

  // Generate V6 routes
  for (int i = 0; i < scale.fibV6Size; i++) {
    std::string prefix;
    if (i == 0) {
      prefix = "::/0";
    } else {
      // Generate /64 routes in 2401:db00::/32 space
      int hex3 = (i / 256) % 256;
      int hex4 = i % 256;
      prefix = fmt::format("2401:db00:{:x}:{:x}::/64", hex3, hex4);
    }

    auto nexthops = createNextHops(scale.v6Nexthops, true);
    auto routeFields = createRouteFields(prefix, nexthops);
    fibContainer.fibV6()[prefix] = std::move(routeFields);
  }

  return fibContainer;
}

RouteFields FsdbStateDataFactory::createRouteFields(
    const std::string& prefix,
    const std::vector<NextHopThrift>& nexthops) {
  RouteFields route;

  // Set route prefix
  RoutePrefix routePrefix;
  auto network = folly::IPAddress::createNetwork(prefix);

  routePrefix.prefix() = createBinaryAddress(network.first);
  routePrefix.mask() = network.second;
  route.prefix() = std::move(routePrefix);

  // Set flags (FIB_FLAG_RESOLVED)
  route.flags() = 2;

  // Create forwarding entry
  RouteNextHopEntry fwd;
  fwd.adminDistance() = AdminDistance::DIRECTLY_CONNECTED;
  fwd.action() = RouteForwardAction::NEXTHOPS;
  fwd.nexthops() = nexthops;
  route.fwd() = fwd;

  // Create multi-client nexthops structure
  RouteNextHopsMulti nexthopsmulti;
  nexthopsmulti.lowestAdminDistanceClientId() = ClientID::BGPD;
  nexthopsmulti.client2NextHopEntry()[ClientID::BGPD] = fwd;
  route.nexthopsmulti() = std::move(nexthopsmulti);

  return route;
}

NextHopThrift FsdbStateDataFactory::createNextHop(
    const std::string& address,
    const std::string& ifName,
    int32_t weight) {
  NextHopThrift nh;

  auto addr = createBinaryAddress(folly::IPAddress(address));
  addr.ifName() = ifName;
  nh.address() = std::move(addr);
  nh.weight() = weight;

  return nh;
}

std::vector<NextHopThrift> FsdbStateDataFactory::createNextHops(
    int count,
    bool isV6,
    const std::string& baseIf) {
  std::vector<NextHopThrift> nexthops;
  nexthops.reserve(count);

  for (int i = 0; i < count; i++) {
    std::string address;
    if (isV6) {
      // Generate V6 nexthop addresses in fe80::/64 link-local space
      address = fmt::format("fe80::{:x}", 1 + i);
    } else {
      // Generate V4 nexthop addresses in 192.168.0.0/16 space
      int octet3 = (i / 256) % 256;
      int octet4 = 1 + (i % 255); // Avoid .0 and .255
      address = fmt::format("192.168.{}.{}", octet3, octet4);
    }

    std::string ifName = fmt::format("{}{}", baseIf, i);
    nexthops.emplace_back(createNextHop(address, ifName, 1));
  }

  return nexthops;
}

BinaryAddress FsdbStateDataFactory::createBinaryAddress(
    const folly::IPAddress& addr) {
  BinaryAddress binaryAddr;

  if (addr.isV4()) {
    auto v4 = addr.asV4();
    binaryAddr.addr() =
        std::string(reinterpret_cast<const char*>(v4.bytes()), 4);
  } else {
    auto v6 = addr.asV6();
    binaryAddr.addr() =
        std::string(reinterpret_cast<const char*>(v6.bytes()), 16);
  }

  return binaryAddr;
}

FibScale FsdbStateDataFactory::getRoleScale(RoleSelector role) {
  // Data scale based on role.
  static const std::map<RoleSelector, FibScale> roleScales = {
      // Role, FIBv4 size,	FIBv6 size,	v4 nexthops,	v6 nexthops
      {RTSW, {1, 3500, 0, 32}},
      {FTSW, {1, 2000, 0, 32}},
      {STSW, {1, 2000, 0, 8}},
      {RSW, {150, 2000, 8, 8}},
      {FSW, {150, 1000, 50, 100}},
      {SSW, {150, 1064, 50, 64}},
      {XSW, {150, 512, 40, 128}},
      {MA, {13000, 16000, 50, 50}},
      {FA, {13000, 20000, 100, 100}},
      // Default fallback
      {Minimal, {1, 1, 1, 1}},
      {MaxScale, {100, 100, 10, 10}},
  };

  auto it = roleScales.find(role);
  if (it != roleScales.end()) {
    return it->second;
  }

  return {1, 1, 1, 1};
}

// AgentStatsDataFactory Implementation
TaggedOperState FsdbStatsDataFactory::getStateUpdate(
    int /* unused */,
    bool /* unused */) {
  TaggedOperState state;
  OperState chunk;
  std::vector<std::string> basePath;
  chunk.protocol() = protocol_;

  auto fsdbRoot = buildFsdbOperStatsRoot();

  chunk.contents() = facebook::fboss::thrift_cow::serialize<
      apache::thrift::type_class::structure>(protocol_, fsdbRoot);

  state.state() = std::move(chunk);
  state.path()->path() = std::move(basePath);
  return state;
}

fsdb::FsdbOperStatsRoot FsdbStatsDataFactory::buildFsdbOperStatsRoot() {
  fsdb::FsdbOperStatsRoot root;

  // Build agent stats data
  root.agent() = buildAgentStats();
  return root;
}

AgentStats FsdbStatsDataFactory::buildAgentStats() {
  AgentStats agentStats;
  AgentStatsScale scale = getRoleScale(selector_);
  int64_t baseTimestamp = 1755207740; // Pick a random timestamp

  // Generate hwPortStats
  for (int i = 0; i < scale.hwPortStatsCount; i++) {
    std::string portName = fmt::format(
        "eth{}/{}/{}", (i / 100) + 1, ((i / 10) % 10) + 1, (i % 10) + 1);
    agentStats.hwPortStats()[portName] =
        createHwPortStats(portName, baseTimestamp, i);
  }

  // Generate phyStats
  for (int i = 0; i < scale.phyStatsCount; i++) {
    std::string portName = fmt::format(
        "eth{}/{}/{}", (i / 100) + 1, ((i / 10) % 10) + 1, (i % 10) + 1);
    agentStats.phyStats()[portName] = createPhyStats(baseTimestamp, i);
  }

  // Generate sysPortStats
  for (int i = 0; i < scale.sysPortStatsCount; i++) {
    std::string portName = fmt::format(
        "edsw{:03d}.n{:03d}.l{:03d}.nao{}:rcy{}/{}/{}",
        (i / 100) + 1,
        i % 10,
        201,
        (i % 4) + 1,
        (i / 10) + 1,
        (i / 5) + 1,
        i + 447);
    auto sysPortStatsData = createSysPortStats(portName, baseTimestamp, i);
    agentStats.sysPortStats()[portName] = sysPortStatsData;
  }

  // Populate sysPortStatsMap[0] with the same data as sysPortStats
  if (!agentStats.sysPortStats()->empty()) {
    agentStats.sysPortStatsMap()[0] = *agentStats.sysPortStats();
  }

  return agentStats;
}

HwPortStats FsdbStatsDataFactory::createHwPortStats(
    const std::string& portName,
    int64_t baseTimestamp,
    int hwPortNum) {
  HwPortStats stats;

  // Set production-like values
  stats.inBytes_() = 2011249318588522LL + (hwPortNum % 1000000000);
  stats.inUnicastPkts_() = 1682544273466LL + (hwPortNum % 1000000);
  stats.inMulticastPkts_() = 142941 + (hwPortNum % 1000);
  stats.inBroadcastPkts_() = 7096 + (hwPortNum % 100);
  stats.inDiscards_() = 1 + (hwPortNum % 10);
  stats.inErrors_() = 0;
  stats.inPause_() = 0;
  stats.inIpv4HdrErrors_() = -1;
  stats.inIpv6HdrErrors_() = -1;
  stats.inDstNullDiscards_() = 786020 + (hwPortNum % 10000);
  stats.inDiscardsRaw_() = 786013 + (hwPortNum % 10000);

  stats.outBytes_() = 2809469388326760LL + (hwPortNum % 1000000000);
  stats.outUnicastPkts_() = 2162624624668LL + (hwPortNum % 1000000);
  stats.outMulticastPkts_() = 142931 + (hwPortNum % 1000);
  stats.outBroadcastPkts_() = 7204 + (hwPortNum % 100);
  stats.outDiscards_() = 0;
  stats.outErrors_() = 0;
  stats.outPause_() = 0;
  stats.outCongestionDiscardPkts_() = 0;
  stats.wredDroppedPackets_() = 0;
  stats.outEcnCounter_() = 0;

  // Queue stats (8 queues)
  for (int j = 0; j < 8; j++) {
    stats.queueOutDiscardBytes_()[j] = 0;
    stats.queueOutPackets_()[j] = ((hwPortNum * 8 + j) % 1000000000LL);
    stats.queueOutDiscardPackets_()[j] = 0;
    stats.queueWatermarkBytes_()[j] = 5000 + ((hwPortNum * 8 + j) % 10000);
    stats.queueWredDroppedPackets_()[j] = 0;
  }

  // Set some non-zero queue bytes for specific queues
  stats.queueOutBytes_()[0] = 7337314388357LL + (hwPortNum % 1000000);
  stats.queueOutBytes_()[1] = 1086612699647978LL + (hwPortNum % 1000000);
  stats.queueOutBytes_()[2] = 668062206253752LL + (hwPortNum % 1000000);
  stats.queueOutBytes_()[3] = 993070474957000LL + (hwPortNum % 1000000);
  stats.queueOutBytes_()[4] = 0;
  stats.queueOutBytes_()[5] = 0;
  stats.queueOutBytes_()[6] = 54389857992826LL + (hwPortNum % 1000000);
  stats.queueOutBytes_()[7] = 123722831 + (hwPortNum % 1000000);

  // FEC stats
  stats.fecCorrectableErrors() = 1302397966 + (hwPortNum % 10000);
  stats.fecUncorrectableErrors() = 12 + (hwPortNum % 100);
  stats.fecCorrectedBits_() = 1227967806 + (hwPortNum % 10000);

  // FEC codewords
  for (int j = 0; j < 16; j++) {
    stats.fecCodewords_()[j] =
        (j == 10 || j == 13 || j == 15) ? ((hwPortNum + j) % 5) : 0;
  }

  // PFC stats
  stats.inPfcCtrl_() = -1;
  stats.outPfcCtrl_() = -1;
  for (int j = 0; j < 8; j++) {
    stats.inPfc_()[j] = 0;
    stats.inPfcXon_()[j] = 0;
    stats.outPfc_()[j] = 0;
  }

  // Set timestamp and port name
  stats.timestamp_() = baseTimestamp + (hwPortNum % 100);
  stats.portName_() = portName;
  stats.inLabelMissDiscards_() = -1;
  stats.inCongestionDiscards_() = 0;
  stats.logicalPortId() = 193 + (hwPortNum % 1000);

  return stats;
}

PhyStats FsdbStatsDataFactory::createPhyStats(
    int64_t baseTimestamp,
    int phyPortNum) {
  PhyStats stats;

  // Create line side stats
  stats.line() = createPhySideStats(Side::LINE, phyPortNum);
  stats.linkFlapCount() = phyPortNum % 10;
  stats.ioStats() = createIOStats();
  stats.timeCollected() = baseTimestamp + (phyPortNum % 100);

  return stats;
}

PhySideStats FsdbStatsDataFactory::createPhySideStats(
    Side side,
    int portIndex) {
  PhySideStats sideStats;
  sideStats.side() = side;
  sideStats.pcs() = createPcsStats(portIndex);
  sideStats.pmd() = createPmdStats(portIndex);
  return sideStats;
}

PcsStats FsdbStatsDataFactory::createPcsStats(int portIndex) {
  PcsStats pcsStats;
  pcsStats.rsFec() = createRsFecInfo(portIndex);
  return pcsStats;
}

RsFecInfo FsdbStatsDataFactory::createRsFecInfo(int portIndex) {
  RsFecInfo rsFec;
  rsFec.correctedCodewords() = 10418410 + (portIndex % 100000);
  rsFec.uncorrectedCodewords() = 0;
  rsFec.correctedBits() = 9886733 + (portIndex % 100000);
  rsFec.preFECBer() = 8.9e-11;
  rsFec.fecTail() = 0;
  rsFec.maxSupportedFecTail() = 15;

  for (int j = 0; j < 16; j++) {
    rsFec.codewordStats()[j] = 0;
  }

  return rsFec;
}

PmdStats FsdbStatsDataFactory::createPmdStats(int portIndex) {
  PmdStats pmdStats;

  for (int j = 0; j < 4; j++) {
    pmdStats.lanes()[j] = createLaneStats(j, portIndex);
  }

  return pmdStats;
}

LaneStats FsdbStatsDataFactory::createLaneStats(int16_t laneId, int portIndex) {
  LaneStats laneStats;
  laneStats.lane() = laneId;
  laneStats.signalDetectChangedCount() = 1 + ((portIndex + laneId) % 3);
  laneStats.cdrLockChangedCount() = 1;
  return laneStats;
}

IOStats FsdbStatsDataFactory::createIOStats() {
  IOStats ioStats;
  ioStats.readDownTime() = 0;
  ioStats.writeDownTime() = 0;
  ioStats.numReadAttempted() = 0;
  ioStats.numReadFailed() = 0;
  ioStats.numWriteAttempted() = 0;
  ioStats.numWriteFailed() = 0;
  return ioStats;
}

HwSysPortStats FsdbStatsDataFactory::createSysPortStats(
    const std::string& portName,
    int64_t baseTimestamp,
    int sysPortNum) {
  HwSysPortStats stats;

  // Set production-like values
  stats.queueOutDiscardBytes_()[0] = 0;
  stats.queueOutBytes_()[0] = 0;
  stats.queueWatermarkBytes_()[0] = 0;
  stats.queueCreditWatchdogDeletedPackets_()[0] = 0;
  stats.queueLatencyWatermarkNsec_()[0] = 800000;

  stats.queueOutDiscardBytes_()[1] = 0;
  stats.queueOutBytes_()[1] = 162728800 + (sysPortNum % 1000000);
  stats.queueWatermarkBytes_()[1] = 144 + (sysPortNum % 100);
  stats.queueCreditWatchdogDeletedPackets_()[1] =
      3386812 + (sysPortNum % 10000);
  stats.queueLatencyWatermarkNsec_()[1] = 800000;

  stats.timestamp_() = baseTimestamp + (sysPortNum % 100);
  stats.portName_() = portName;

  return stats;
}

AgentStatsScale FsdbStatsDataFactory::getRoleScale(RoleSelector role) {
  // Stats scale based on role.
  static const std::map<RoleSelector, AgentStatsScale> roleScales = {
      // Role, hwPortStats,	phyStats,	sysPortStats
      {FA, {108, 108, 0}},
      {RSW, {24, 24, 0}},
      {RTSW, {64, 64, 0}},
      {RDSW, {203, 197, 683}},
      {FDSW, {1024, 1024, 0}},
      {SDSW, {800, 800, 0}},
      {EDSW, {183, 177, 663}},
      // Default fallback
      {Minimal, {1, 1, 1}},
      {MaxScale, {10, 10, 10}},
  };

  auto it = roleScales.find(role);
  if (it != roleScales.end()) {
    return it->second;
  }

  return {1, 1, 1};
}

} // namespace facebook::fboss::test_data
