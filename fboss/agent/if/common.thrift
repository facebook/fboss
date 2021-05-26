namespace cpp2 facebook.fboss
namespace go neteng.fboss.common
namespace php fboss
namespace py neteng.fboss.common
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.common
namespace rust facebook.fboss

include "fboss/agent/if/mpls.thrift"
include "common/network/if/Address.thrift"

struct NextHopThrift {
  1: Address.BinaryAddress address,
  // Default weight of 0 represents an ECMP route.
  // This default is chosen for two reasons:
  // 1) We rely on the arithmetic properties of 0 for ECMP vs UCMP route
  //    resolution calculations. A 0 weight next hop being present at a variety
  //    of layers in a route resolution tree will cause the entire route
  //    resolution to use ECMP.
  // 2) A client which does not set a value will result in
  //    0 being populated even with strange behavior in the client language
  //    which is consistent with C++
  2: i32 weight = 0,
  // MPLS encapsulation information for IP->MPLS and MPLS routes
  3: optional mpls.MplsAction mplsAction,
}
