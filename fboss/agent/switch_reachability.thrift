#
# Copyright 2004-present Facebook. All Rights Reserved.
#
namespace py neteng.fboss.switch_reachability
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.switch_reachability
namespace cpp2 facebook.fboss.switch_reachability
namespace go neteng.fboss.switch_reachability

struct SwitchReachability {
  // Keep track of the set of fabric ports to reach remote switches
  // using a unique ID
  1: map<i64, set<string>> fabricPortGroupMap = {};
  // Map switchId to the fabric port set over which its reachable.
  // This way, avoid the overhead of repeating the fabric port set
  // for each switchId
  2: map<i64, i64> switchIdToFabricPortGroupMap = {};
}
