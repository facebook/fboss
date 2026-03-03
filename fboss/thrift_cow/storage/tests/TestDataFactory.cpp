// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/TestDataFactory.h"
#include <fmt/format.h>
#include <folly/IPAddress.h>
#include <folly/base64.h>

#include "fboss/thrift_cow/nodes/Serializer.h"

// Additional includes for AgentStats
#include "fboss/agent/gen-cpp2/agent_stats_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/lib/if/gen-cpp2/io_stats_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

// Include StateGenerator for remote system ports and interfaces
#include "fboss/fsdb/benchmarks/StateGenerator.h"

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

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

  // Populate remote system ports and interfaces for RDSW and EDSW roles
  populateRemoteSystemPortsAndInterfaces(switchState);

  return switchState;
}

FibContainerFields FsdbStateDataFactory::buildFibData(void) {
  FibContainerFields fibContainer;
  SwitchStateScale scale = getRoleScale(selector_);

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
    std::string addrStr = v4.str();
    binaryAddr.addr() = addrStr;
  } else {
    auto v6 = addr.asV6();
    std::string addrStr = v6.str();
    binaryAddr.addr() = addrStr;
  }

  return binaryAddr;
}

void FsdbStateDataFactory::populateRemoteSystemPortsAndInterfaces(
    SwitchState& switchState) {
  SwitchStateScale scale = getRoleScale(selector_);
  int numDsfNbrAddresses = 3;

  // Only populate for RDSW and EDSW roles
  if (scale.remoteSystemPortMapSize == 0) {
    return;
  }

  std::string switchIdList = "Id:648";

  // Create remote system port map
  std::map<int64_t, state::SystemPortFields> remoteSystemPortMap;
  for (int i = 0; i < scale.remoteSystemPortMapSize; i++) {
    auto sysPortFields = facebook::fboss::fsdb::test::fillSystemPortMap(648, i);
    remoteSystemPortMap.emplace(i, std::move(sysPortFields));
  }

  // Create remote interface map
  std::map<int32_t, state::InterfaceFields> remoteInterfaceMap;
  for (int i = 0; i < scale.remoteInterfaceMapSize; i++) {
    auto interfaceFields =
        facebook::fboss::fsdb::test::fillInterfaceMap(i, numDsfNbrAddresses);
    remoteInterfaceMap.emplace(i, std::move(interfaceFields));
  }

  // Populate the maps in switchState
  switchState.remoteSystemPortMaps()[switchIdList] =
      std::move(remoteSystemPortMap);
  switchState.remoteInterfaceMaps()[switchIdList] =
      std::move(remoteInterfaceMap);
}
// Scale configurations for different deployment roles
SwitchStateScale FsdbStateDataFactory::getRoleScale(RoleSelector role) {
  static const std::map<RoleSelector, SwitchStateScale> roleScales = {
      // Role, fibV4Size, fibV6Size, v4Nexthops, v6Nexthops,
      // remoteSystemPortMapSize, remoteInterfaceMapSize
      {RTSW, {1, 3500, 0, 32, 0, 0}},
      {FTSW, {1, 2000, 0, 32, 0, 0}},
      {STSW, {1, 2000, 0, 8, 0, 0}},
      {RSW, {150, 2000, 8, 8, 0, 0}},
      {FSW, {150, 1000, 50, 100, 0, 0}},
      {SSW, {875, 4200, 50, 64, 0, 0}},
      {XSW, {150, 512, 40, 128, 0, 0}},
      {MA, {13000, 16000, 50, 50, 0, 0}},
      {FA, {13000, 20000, 100, 100, 0, 0}},
      {RDSW, {1, 22000, 0, 2, 21750, 21750}},
      {EDSW, {1, 22000, 0, 1, 21750, 21750}},
      // Default fallback
      {Minimal, {1, 1, 1, 1, 0, 0}},
      {MaxScale, {100, 100, 10, 10, 0, 0}},
  };

  auto it = roleScales.find(role);
  if (it != roleScales.end()) {
    return it->second;
  }

  return {1, 1, 1, 1, 0, 0};
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
      {RDSW, {203, 197, 21766}},
      {FDSW, {1024, 1024, 0}},
      {SDSW, {800, 800, 0}},
      {EDSW, {183, 177, 21766}},
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

// BgpRibMapDataGenerator implementation
TaggedOperState BgpRibMapDataGenerator::getStateUpdate(
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

fsdb::FsdbOperStateRoot BgpRibMapDataGenerator::buildFsdbOperStateRoot() {
  fsdb::FsdbOperStateRoot root;

  root.agent() = fsdb::AgentData{};
  root.bgp() = buildBgpData();
  root.openr() = fsdb::OpenrData{};

  return root;
}

BgpRibMapScale BgpRibMapDataGenerator::getScale(RoleSelector role) {
  static const std::map<RoleSelector, BgpRibMapScale> roleScales = {
      // Role, ribV4EntryCount, ribV6EntryCount, bestPathsPerEntry,
      // communitiesPerPath, asPathSegments, extCommunitiesPerPath
      {RSW, {317, 754, 4, 13, 2, 0}},
      {FSW, {555, 901, 24, 14, 1, 0}},
      {FA, {8890, 14221, 10, 12, 1, 1}},
      // Default fallback
      {Minimal, {5, 5, 1, 5, 1, 0}},
      {MaxScale, {10000, 15000, 10, 12, 2, 1}},
  };

  auto it = roleScales.find(role);
  if (it == roleScales.end()) {
    // default to Minimal
    it = roleScales.find(Minimal);
  }
  return it->second;
}

fsdb::BgpData BgpRibMapDataGenerator::buildBgpData() {
  fsdb::BgpData bgpData;
  BgpRibMapScale scale = getScale(selector_);

  std::map<std::string, TRibEntry> ribMap;

  // Generate IPv4 RIB entries
  for (int i = 0; i < scale.ribV4EntryCount; i++) {
    TRibEntry entry = buildTRibEntry(scale, i, false);
    std::string prefixKey = createPrefixKey(*entry.prefix());
    ribMap[prefixKey] = std::move(entry);
  }

  // Generate IPv6 RIB entries
  for (int i = 0; i < scale.ribV6EntryCount; i++) {
    TRibEntry entry = buildTRibEntry(scale, i, true);
    std::string prefixKey = createPrefixKey(*entry.prefix());
    ribMap[prefixKey] = std::move(entry);
  }

  bgpData.ribMap() = std::move(ribMap);
  return bgpData;
}

TRibEntry BgpRibMapDataGenerator::buildTRibEntry(
    const BgpRibMapScale& scale,
    int index,
    bool isV6) {
  TRibEntry entry;

  auto prefix = createPrefix(index, isV6);
  entry.prefix() = prefix;

  // Generate paths map with "best" group
  std::vector<neteng::fboss::bgp::thrift::TBgpPath> bestPaths;
  for (int j = 0; j < scale.bestPathsPerEntry; j++) {
    auto path = createBgpPath(
        index,
        j,
        scale.communitiesPerPath,
        scale.asPathSegments,
        scale.extCommunitiesPerPath);
    bestPaths.emplace_back(std::move(path));
  }

  std::map<std::string, std::vector<neteng::fboss::bgp::thrift::TBgpPath>>
      pathsMap;
  pathsMap["best"] = std::move(bestPaths);

  // Set best_group
  entry.best_group() = "best";

  // Set best_next_hop (use first path's next_hop)
  if (!pathsMap["best"].empty()) {
    *entry.best_next_hop() = *pathsMap["best"][0].next_hop();
  }

  entry.paths() = std::move(pathsMap);

  // Create prefix key string
  std::string prefixKey = createPrefixKey(prefix);

  return entry;
}

facebook::neteng::fboss::bgp_attr::TIpPrefix
BgpRibMapDataGenerator::createPrefix(int index, bool isV6) {
  facebook::neteng::fboss::bgp_attr::TIpPrefix prefix;

  if (isV6) {
    // IPv6 prefix
    prefix.afi() = facebook::neteng::fboss::bgp_attr::TBgpAfi::AFI_IPV6;
    int hex3 = (index / 512) % 256;
    int hex4 = index % 256;
    std::string prefixStr = fmt::format("2401:db00:{:x}:{:x}::/64", hex3, hex4);
    auto network = folly::IPAddress::createNetwork(prefixStr);
    auto bytes = network.first.asV6().toByteArray();
    std::string encoded = folly::base64Encode(
        std::string_view(
            reinterpret_cast<const char*>(bytes.data()), bytes.size()));
    prefix.prefix_bin() = encoded;
    prefix.num_bits() = 64;
  } else {
    // IPv4 prefix
    prefix.afi() = facebook::neteng::fboss::bgp_attr::TBgpAfi::AFI_IPV4;
    int octet2 = (index / 256) % 256;
    int octet3 = index % 256;
    std::string prefixStr = fmt::format("10.{}.{}.0/24", octet2, octet3);
    auto network = folly::IPAddress::createNetwork(prefixStr);
    auto bytes = network.first.asV4().toByteArray();
    std::string encoded = folly::base64Encode(
        std::string_view(
            reinterpret_cast<const char*>(bytes.data()), bytes.size()));
    prefix.prefix_bin() = encoded;
    prefix.num_bits() = 24;
  }

  return prefix;
}

neteng::fboss::bgp::thrift::TBgpPath BgpRibMapDataGenerator::createBgpPath(
    int entryIndex,
    int pathIndex,
    int numCommunities,
    int numAsPathSegments,
    int numExtCommunities) {
  neteng::fboss::bgp::thrift::TBgpPath path;

  // Create next_hop
  facebook::neteng::fboss::bgp_attr::TIpPrefix nextHop;
  nextHop.afi() = facebook::neteng::fboss::bgp_attr::TBgpAfi::AFI_IPV6;
  std::string nextHopStr =
      fmt::format("fe80::{:x}:{:x}", entryIndex % 65536, pathIndex % 65536);
  auto nextHopAddr = folly::IPAddress(nextHopStr);
  auto bytes = nextHopAddr.asV6().toByteArray();
  std::string encoded = folly::base64Encode(
      std::string_view(
          reinterpret_cast<const char*>(bytes.data()), bytes.size()));
  nextHop.prefix_bin() = encoded;
  nextHop.num_bits() = 128;
  path.next_hop() = nextHop;

  // Create AS path
  facebook::neteng::fboss::bgp_attr::TAsPath asPath;
  for (int i = 0; i < numAsPathSegments; i++) {
    facebook::neteng::fboss::bgp_attr::TAsPathSeg segment;
    segment.seg_type() =
        facebook::neteng::fboss::bgp_attr::TAsPathSegType::AS_SEQUENCE;

    // Add 2-3 ASNs per segment
    int numAsns = 2 + (pathIndex % 2);
    std::vector<int32_t> asns;
    std::vector<int64_t> asns_4_byte;
    for (int j = 0; j < numAsns; j++) {
      int32_t asn = 65000 + ((entryIndex + pathIndex + i + j) % 1000);
      asns.push_back(asn);
      asns_4_byte.push_back(asn);
    }
    segment.asns() = asns;
    segment.asns_4_byte() = asns_4_byte;
    asPath.push_back(segment);
  }
  path.as_path() = asPath;

  // Create communities
  std::vector<facebook::neteng::fboss::bgp_attr::TBgpCommunity> communities;
  for (int i = 0; i < numCommunities; i++) {
    facebook::neteng::fboss::bgp_attr::TBgpCommunity community;
    int16_t asn = 65400 + (i % 100);
    int16_t value = 100 + ((entryIndex + pathIndex + i) % 200);
    community.asn() = asn;
    community.value() = value;
    community.community() = (static_cast<int32_t>(asn) << 16) | value;
    communities.push_back(community);
  }
  path.communities() = communities;

  // Create extended communities
  if (numExtCommunities > 0) {
    std::vector<neteng::fboss::bgp::thrift::TBgpExtCommunity> extCommunities;
    for (int i = 0; i < numExtCommunities; i++) {
      neteng::fboss::bgp::thrift::TBgpExtCommunity extComm;
      neteng::fboss::bgp::thrift::TBgpExtCommUnion extCommUnion;
      neteng::fboss::bgp::thrift::TBgpTwoByteAsnExtComm twoByteAsn;
      twoByteAsn.type() = 64;
      twoByteAsn.sub_type() = 4;
      twoByteAsn.asn() = 65000 + (i % 100);
      twoByteAsn.value() = 1000000 + ((entryIndex + pathIndex) % 1000000);
      extCommUnion.two_byte_asn() = twoByteAsn;
      extComm.u() = extCommUnion;
      extCommunities.push_back(extComm);
    }
    path.extCommunities() = extCommunities;
  }

  // Set other path attributes
  path.cluster_list() = std::vector<int64_t>{}; // Empty for most paths
  path.local_pref() = 100;
  path.router_id() = 3232235777 + (entryIndex % 1000); // ~192.0.2.1 range
  path.origin() = 0; // IGP
  path.peer_id() = nextHop;
  path.bestpath_filter_descr() = "";
  path.last_modified_time() = 1700000000000000LL + (entryIndex * 1000);

  // Mark first path as best
  if (pathIndex == 0) {
    path.is_best_path() = true;
  }

  return path;
}

std::string BgpRibMapDataGenerator::createPrefixKey(
    const facebook::neteng::fboss::bgp_attr::TIpPrefix& prefix) {
  auto decoded = folly::base64Decode(*prefix.prefix_bin());
  auto addrResult = folly::IPAddress::tryFromBinary(
      folly::ByteRange(
          reinterpret_cast<const unsigned char*>(decoded.data()),
          decoded.size()));
  if (addrResult.hasValue()) {
    auto& addr = addrResult.value();
    int numBits = *prefix.num_bits();
    return addr.str() + "/" + std::to_string(numBits);
  }
  return "unknown_prefix";
}

} // namespace facebook::fboss::test_data
