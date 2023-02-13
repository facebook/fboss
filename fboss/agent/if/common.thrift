namespace cpp2 facebook.fboss
namespace go neteng.fboss.common
namespace php fboss
namespace py neteng.fboss.common
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.common

include "fboss/agent/if/mpls.thrift"
include "common/network/if/Address.thrift"

struct NextHopThrift {
  1: Address.BinaryAddress address;
  // Default weight of 0 represents an ECMP route.
  // This default is chosen for two reasons:
  // 1) We rely on the arithmetic properties of 0 for ECMP vs UCMP route
  //    resolution calculations. A 0 weight next hop being present at a variety
  //    of layers in a route resolution tree will cause the entire route
  //    resolution to use ECMP.
  // 2) A client which does not set a value will result in
  //    0 being populated even with strange behavior in the client language
  //    which is consistent with C++
  2: i32 weight = 0;
  // MPLS encapsulation information for IP->MPLS and MPLS routes
  3: optional mpls.MplsAction mplsAction;
}

/*
* named next hop group is regular set of next hops but identified by name.
* address of each next hop is recursively resolved.
* if any next hop has MPLS push action, then recursive resolution may expand label stack..
* if any next hop has MPLS swap action, then recursive resolution may expand label stack.
* if any next hop has MPLS php action, then recursive resolution may not expand label stack.
* if any next hop has MPLS pop action, then all next hops must have MPLS pop action, address of nexthop is ignored.
*/
struct NamedNextHopGroup {
  1: required string name;
  2: required list<NextHopThrift> nexthops;
}

/*
 * Forwarding Class
 */
enum ForwardingClass {
  DEFAULT = 0, // internal use only
  CLASS_1 = 1,
  CLASS_2 = 2,
  CLASS_3 = 3,
  CLASS_4 = 4,
  CLASS_5 = 5,
  CLASS_6 = 6,
  CLASS_7 = 7,
}

typedef map<byte, ForwardingClass> DscpToForwardingClassMap
typedef map<ForwardingClass, list<NamedNextHopGroup>> ForwardingClassToNamedNhgs

/*
 * Class based traffic forwarding policy
 */
struct ClassBasedPolicy {
  1: string name;
  2: list<string> defaultNexthopGroups;
  3: ForwardingClassToNamedNhgs class2NextHopGroups;
}

/*
 * Traffic redirection policy
 */
union Policy {
  1: ClassBasedPolicy cbfPolicy;
}

union NamedRouteDestination {
  // list of named next hop groups
  1: list<string> nextHopGroups;
  // traffic redirection policy name
  2: string policyName;
}

struct SystemPortThrift {
  1: i64 portId;
  2: i64 switchId;
  3: string portName; // switchId::portName
  4: i64 coreIndex;
  5: i64 corePortIndex;
  6: i64 speedMbps;
  7: i64 numVoqs;
  9: bool enabled;
  10: optional string qosPolicy;
}
