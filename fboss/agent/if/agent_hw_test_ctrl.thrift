namespace cpp2 facebook.fboss.utility
namespace go neteng.fboss.agent_hw_test_ctrl
namespace php fboss
namespace py neteng.fboss.agent_hw_test_ctrl
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.agent_hw_test_ctrl

include "thrift/annotation/cpp.thrift"
include "fboss/agent/switch_state.thrift"
include "fboss/agent/switch_config.thrift"
include "fboss/agent/if/ctrl.thrift"
include "common/network/if/Address.thrift"

struct NeighborInfo {
  1: bool exists;
  2: bool isProgrammedToCpu;
  3: optional i32 classId;
}

struct CIDRNetwork {
  1: string IPAddress;
  @cpp.Type{name = "uint8_t"}
  2: byte mask;
}

struct RouteInfo {
  1: bool exists;
  2: bool isProgrammedToCpu;
  3: bool isMultiPath;
  4: bool isRouteUnresolvedToClassId;
  5: optional i32 classId;
}

service AgentHwTestCtrl {
  // acl utils begin
  i32 getDefaultAclTableNumAclEntries();

  i32 getAclTableNumAclEntries(1: string name);

  bool isDefaultAclTableEnabled();

  bool isAclTableEnabled(1: string name);

  bool isAclEntrySame(
    1: switch_state.AclEntryFields aclEntry,
    2: string aclTableName,
  );

  bool areAllAclEntriesEnabled();

  bool isStatProgrammedInDefaultAclTable(
    1: list<string> aclEntryNames,
    2: string counterName,
    3: list<switch_config.CounterType> types,
  );

  bool isStatProgrammedInAclTable(
    1: list<string> aclEntryNames,
    2: string counterName,
    3: list<switch_config.CounterType> types,
    4: string tableName,
  );

  bool isMirrorProgrammed(1: switch_state.MirrorFields mirror);

  bool isPortMirrored(1: i32 port, 2: string mirror, 3: bool ingress);

  bool isPortSampled(1: i32 port, 2: string mirror, 3: bool ingress);

  bool isAclEntryMirrored(1: string aclEntry,2: string mirror,3: bool ingress,);

  // neighbor utils
  NeighborInfo getNeighborInfo(1: ctrl.IfAndIP neighbor);

  i32 getHwEcmpSize(1: CIDRNetwork prefix, 2: i32 routerID, 3: i32 sizeInSw);

  void injectFecError(1: list<i32> hwPorts, 2: bool injectCorrectable);

  void injectSwitchReachabilityChangeNotification();

  // route utils
  RouteInfo getRouteInfo(1: ctrl.IpPrefix prefix);
  bool isRouteHit(1: ctrl.IpPrefix prefix);
  void clearRouteHit(1: ctrl.IpPrefix prefix);
  bool isRouteToNexthop(
    1: ctrl.IpPrefix prefix,
    2: Address.BinaryAddress address,
  );
}
