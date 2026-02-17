// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/IPAddress.h>
#include "configerator/structs/neteng/fboss/bgp/if/gen-cpp2/bgp_attr_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/tests/gen-cpp2-thriftpath/thriftpath_test.h" // @manual=//fboss/fsdb/tests:thriftpath_test_thrift-cpp2-thriftpath
#include "neteng/fboss/bgp/if/gen-cpp2/bgp_thrift_types.h"

namespace {
constexpr int kDefaultMapSize = 1 * 1000;
}

namespace facebook::fboss::test_data {

using facebook::fboss::fsdb::OperProtocol;
using facebook::fboss::fsdb::OperState;
using facebook::fboss::fsdb::OtherStruct;
using facebook::fboss::fsdb::TaggedOperState;
using facebook::fboss::fsdb::TestEnum;
using facebook::fboss::fsdb::TestStruct;

// Import switch state types for FIB data
using facebook::fboss::AdminDistance;
using facebook::fboss::ClientID;
using facebook::fboss::NextHopThrift;
using facebook::fboss::RouteForwardAction;
using facebook::fboss::state::FibContainerFields;
using facebook::fboss::state::RouteFields;
using facebook::fboss::state::RouteNextHopEntry;
using facebook::fboss::state::RouteNextHopsMulti;
using facebook::fboss::state::RoutePrefix;
using facebook::fboss::state::SwitchState;
using facebook::network::thrift::BinaryAddress;

// Import AgentStats and related types
using facebook::fboss::AgentStats;
using facebook::fboss::HwPortStats;
using facebook::fboss::HwSysPortStats;
using facebook::fboss::IOStats;
using facebook::fboss::phy::LaneStats;
using facebook::fboss::phy::PcsStats;
using facebook::fboss::phy::PhySideStats;
using facebook::fboss::phy::PhyStats;
using facebook::fboss::phy::PmdStats;
using facebook::fboss::phy::RsFecInfo;
using facebook::fboss::phy::Side;

// Import BGP types
using TRibEntry = neteng::fboss::bgp::thrift::TRibEntry;

enum RoleSelector {
  Minimal = 0,
  MaxScale = 1,
  RTSW = 2,
  FTSW = 3,
  STSW = 4,
  RSW = 5,
  FSW = 6,
  SSW = 7,
  XSW = 8,
  MA = 9,
  FA = 10,
  RDSW = 11,
  FDSW = 12,
  SDSW = 13,
  EDSW = 14,
};

struct SwitchStateScale {
  int fibV4Size;
  int fibV6Size;
  int v4Nexthops;
  int v6Nexthops;
  int remoteSystemPortMapSize;
  int remoteInterfaceMapSize;
};

struct AgentStatsScale {
  int hwPortStatsCount;
  int phyStatsCount;
  int sysPortStatsCount;
};

struct BgpRibMapScale {
  int ribV4EntryCount;
  int ribV6EntryCount;
  int bestPathsPerEntry;
  int communitiesPerPath;
  int asPathSegments;
  int extCommunitiesPerPath;
};

class IDataGenerator {
 public:
  virtual ~IDataGenerator() = default;

  virtual TaggedOperState getStateUpdate(int version, bool minimal) = 0;
};

class TestDataFactory : public IDataGenerator {
 public:
  using RootT = TestStruct;

  explicit TestDataFactory(
      RoleSelector selector,
      int scaleFactor = kDefaultMapSize)
      : selector_(selector), scaleFactor_(scaleFactor) {}

  TaggedOperState getStateUpdate(int version, bool minimal) override;

 protected:
  OtherStruct
  buildMinimalTestData(int version, int key, std::vector<std::string>& path);

  TestStruct buildTestData(int version, std::vector<std::string>& /* path */);

  RoleSelector selector_;
  int scaleFactor_;
  OperProtocol protocol_{OperProtocol::COMPACT};
};

class FsdbStateDataFactory : public IDataGenerator {
 public:
  using RootT = fsdb::FsdbOperStateRoot;

  explicit FsdbStateDataFactory(RoleSelector selector) : selector_(selector) {}

  TaggedOperState getStateUpdate(int version, bool minimal) override;

 protected:
  fsdb::FsdbOperStateRoot buildFsdbOperStateRoot();
  SwitchState buildSwitchState();
  FibContainerFields buildFibData();
  SwitchStateScale getRoleScale(RoleSelector role);

  RouteFields createRouteFields(
      const std::string& prefix,
      const std::vector<NextHopThrift>& nexthops);
  NextHopThrift createNextHop(
      const std::string& address,
      const std::string& ifName = "eth0",
      int32_t weight = 1);
  std::vector<NextHopThrift> createNextHops(
      int count,
      bool isV6 = false,
      const std::string& baseIf = "eth");
  BinaryAddress createBinaryAddress(const folly::IPAddress& addr);

  // Helper methods for remote system ports and remote interfaces
  void populateRemoteSystemPortsAndInterfaces(SwitchState& switchState);

  RoleSelector selector_;
  OperProtocol protocol_{OperProtocol::COMPACT};
};

class FsdbStatsDataFactory : public IDataGenerator {
 public:
  using RootT = fsdb::FsdbOperStatsRoot;

  explicit FsdbStatsDataFactory(RoleSelector selector) : selector_(selector) {}

  TaggedOperState getStateUpdate(int version, bool minimal) override;

 protected:
  fsdb::FsdbOperStatsRoot buildFsdbOperStatsRoot();
  AgentStats buildAgentStats();
  AgentStatsScale getRoleScale(RoleSelector role);

  // Helper methods to create AgentStats structures
  HwPortStats createHwPortStats(
      const std::string& portName,
      int64_t baseTimestamp,
      int hwPortNum);
  PhyStats createPhyStats(int64_t baseTimestamp, int phyPortNum);
  HwSysPortStats createSysPortStats(
      const std::string& portName,
      int64_t baseTimestamp,
      int sysPortNum);

  // Helper methods for internal structures
  RsFecInfo createRsFecInfo(int portIndex);
  PhySideStats createPhySideStats(Side side, int portIndex);
  PcsStats createPcsStats(int portIndex);
  PmdStats createPmdStats(int portIndex);
  LaneStats createLaneStats(int16_t laneId, int portIndex);
  IOStats createIOStats();

  RoleSelector selector_;
  OperProtocol protocol_{OperProtocol::COMPACT};
};

class BgpRibMapDataGenerator : public IDataGenerator {
 public:
  using RootT = fsdb::FsdbOperStateRoot;

  explicit BgpRibMapDataGenerator(RoleSelector selector)
      : selector_(selector) {}

  TaggedOperState getStateUpdate(int version, bool minimal) override;

 protected:
  fsdb::FsdbOperStateRoot buildFsdbOperStateRoot();
  fsdb::BgpData buildBgpData();
  BgpRibMapScale getScale(RoleSelector role);

  // Helper methods to create BGP structures
  TRibEntry buildTRibEntry(const BgpRibMapScale& scale, int index, bool isV6);
  facebook::neteng::fboss::bgp_attr::TIpPrefix createPrefix(
      int index,
      bool isV6);
  neteng::fboss::bgp::thrift::TBgpPath createBgpPath(
      int entryIndex,
      int pathIndex,
      int numCommunities,
      int numAsPathSegments,
      int numExtCommunities);
  std::string createPrefixKey(
      const facebook::neteng::fboss::bgp_attr::TIpPrefix& prefix);

  RoleSelector selector_;
  OperProtocol protocol_{OperProtocol::COMPACT};
};

} // namespace facebook::fboss::test_data
