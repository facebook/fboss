/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

include "configerator/structs/neteng/fboss/bgp/if/bgp_attr.thrift"
include "configerator/structs/neteng/bgp_policy/thrift/rib_policy.thrift"
include "thrift/annotation/cpp.thrift"
include "thrift/annotation/thrift.thrift"

package "facebook.com/neteng/fboss/bgp/if/bgp_route_types"

namespace cpp2 facebook.neteng.fboss.bgp.thrift
namespace py neteng.fboss.bgp_route_types
namespace py.asyncio neteng.fboss.asyncio.bgp_route_types
namespace py3 neteng.fboss
namespace go neteng.fboss.bgp_route_types

/*
 * BGP extended community types.
 *
 * These represent extended communities as exchanged between applications,
 * not the wire format. Represented as a union for extensibility.
 */

enum TBgpTwoByteAsnExtCommType {
  LINK_BANDWIDTH_TYPE = 0x40,
}

enum TBgpTwoByteAsnExtCommSubType {
  LINK_BANDWIDTH_SUB_TYPE = 0x4,
  LINK_BANDWIDTH_SUB_TYPE_TRANSITIVE = 0x0,
}

struct TBgpTwoByteAsnExtComm {
  1: i16 type;
  2: i16 sub_type;
  3: i32 asn;
  4: i64 value;
}

struct TBgpRawExtComm {
  1: i64 value_low;
  2: i64 value_high;
}

union TBgpExtCommUnion {
  1: TBgpTwoByteAsnExtComm two_byte_asn;
  // Pending fix for D union implementation
  // 2: TBgpRawExtComm raw_values;
}

/**
 * This could theoretically be collapsed into a union, but leaving this as a
 * struct in case we want to add more fields here in the future.
*/

struct TBgpExtCommunity {
  1: TBgpExtCommUnion u;
}

struct TBgpAggregator {
  1: i32 asn;
  2: string ip;
}

/**
 * Single BGP path for a Rib entry, points to the peer who advertised it
 * This includes all fields in BgpAttributesC.
 */
struct TBgpPath {
  1: bgp_attr.TIpPrefix next_hop;
  2: bgp_attr.TAsPath as_path;
  3: optional list<bgp_attr.TBgpCommunity> communities;
  4: optional i64 originator_id;
  5: optional list<i64> cluster_list;
  6: optional i32 local_pref;
  7: optional i64 router_id;
  8: optional i32 origin;
  9: optional bgp_attr.TIpPrefix peer_id;
  /* BGP Path selection/rejection reason */
  10: optional string bestpath_filter_descr;
  11: optional list<TBgpExtCommunity> extCommunities;
  12: i64 last_modified_time; // time is in microseconds
  13: optional i64 path_id; // received path ID
  /*
   * Name of the policy (including policy and term name) which causes a
   * route (along with BGP attributes) to get accepted, rejected or modified
   * when applied to it.
   * Routes received from a peer may get rejected by a policy or accepted
   * to be programmed into RIB if a policy matches. Policy information
   * will be shown in the output of cli command:
   * fboss bgp postfilter-received <peer>.
   * Similarly, routes advertised to a peer may get accepted by a policy or
   * modified by it before it gets advertised to a peer. Policy information
   * will be shown in the output of cli command:
   * fboss bgp postfilter-advertised n<peer>.
   */
  14: optional string policy_name;
  // deprecated: use med (field 21) instead
  @thrift.DeprecatedUnvalidatedAnnotations{items = {"deprecated": "1"}}
  15: optional i32 multi_exit_disc;
  16: optional bool atomic_aggregate;
  17: optional TBgpAggregator aggregator;
  18: optional string peer_description;
  19: optional i64 next_hop_weight;
  // Used to indicate best path. This field is introduced so
  // fboss cli uses best path computed in bgp++ RIB instead of
  // using best_next_hop to deduce best path.
  20: optional bool is_best_path;
  21: optional i64 med;
  22: optional i32 weight;
  23: bool in_update = false;
  24: bool in_withdraw = true;
  // Used to encapsulate topology information for the control plane
  // to make routing decisions, e.g., NSF GAR
  @cpp.Type{template = "std::unordered_map"}
  25: optional map<string, i64> topologyInfo;
  26: optional i64 igp_cost;
  // path_id above would be received path ID. A speaker can also allocate
  // its own path ID to send to peers, which would go in this field
  27: optional i64 path_id_to_send;
}

/**
 * RIB entry along with host information that we are querying against
 */
struct TRibEntryWithHost {
  1: list<TRibEntry> tRibEntries;
  2: string host;
  3: string ip;
  4: string oobName;
}

/**
 * RIB entry binds a prefix to multiple ECMP Bgp paths
 * Afi is inferred from prefix.afi
 */
struct TRibEntry {
  1: bgp_attr.TIpPrefix prefix;
  /** maps a group to the list of paths */
  2: map<string, list<TBgpPath>> paths;
  3: string best_group;
  4: bgp_attr.TIpPrefix best_next_hop;
  /*
   * If the path selection of the route is overridden by CPS,
   * the corresponding active criteria is set here
   * see getActivePathSelectionCriteria
   */
  5: optional rib_policy.TPathSelector active_cps_criteria;
  /*
   * Indicates path selection is pending for this entry.
   * When true, IGP cost may have changed but best-path hasn't been
   * recalculated yet. This helps identify transient states where
   * the displayed best-path may not reflect current IGP costs.
   */
  6: optional bool path_selection_pending;
  /*
   * RIB version when this entry was last modified. This is a monotonically
   * increasing counter that increments whenever a material routing change
   * occurs (best path or multipath changes). Used for tracking routing
   * table version per prefix.
   */
  7: optional i64 rib_version;
  /*
   * If the route's weights are overridden by CTE (route attribute policy),
   * the corresponding active UCMP action is set here
   */
  8: optional rib_policy.TRouteAttributeUcmpAction active_cte_ucmp_action;
  /*
   * Convenience copy of the selected best-path entry from `paths`.
   * Lets FSDB subscribers fetch only the best path (e.g. via
   * `ribMap/<prefix>/best_path`) without subscribing to the full `paths`
   * map, which is significantly larger.
   *
   * Unset when no path is currently "best" -- e.g. when CPS native criteria
   * (bgp_native_path_selection_min_nexthop / min_agg_lbw) is violated and
   * the entry is multipath-only with no bestpath, or when `bestpath` has
   * not yet been computed for this prefix.
   *
   * The `is_best_path` flag on this copy is always set to true.
   */
  9: optional TBgpPath best_path;
}

/**
 * A single prefix in partial-drain state due to MNH violation.
 * When relax MNH is enabled and a prefix violates the min-nexthop
 * threshold, the prefix is advertised with drain community 65446:10
 */
struct TPartiallyDrainedPrefix {
  /** CIDR prefix currently in partial-drain state */
  1: bgp_attr.TIpPrefix prefix;
  /** Number of valid paths (nexthops) currently available for this prefix */
  2: i32 path_count;
  /** Configured MNH threshold */
  3: i32 mnh_threshold;
}

/**
 * Device-level partial-drain status
 */
struct TPartialDrainStatus {
  /** True if one or more prefixes are in partial-drain state */
  1: bool is_partially_drained;
  /** Number of prefixes currently violating MNH with relax=true */
  2: i32 num_affected_prefixes;
  /** Monotonic counter incremented each time the device enters or exits
   *  partial-drain state (is_partially_drained flips) in the current
   *  incarnation of BGP */
  3: i64 partial_drain_transition_count;
}

/**
 * Composite partial-drain state
 */
struct TPartialDrainState {
  /** Device-level summary — subscribe to this subpath for lightweight gating */
  1: TPartialDrainStatus partial_drain_state;
  /** Per-prefix detail — subscribe to full path when prefix-level visibility needed */
  2: list<TPartiallyDrainedPrefix> drained_prefixes;
}
