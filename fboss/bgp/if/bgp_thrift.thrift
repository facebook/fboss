cpp_include "folly/container/F14Map.h"

include "common/fb303/if/fb303.thrift"
include "configerator/structs/neteng/bgp_policy/thrift/rib_policy.thrift"
include "configerator/structs/neteng/bgp_policy/thrift/bgp_policy.thrift"
include "fboss/bgp/if/bgp_config.thrift"
include "configerator/structs/neteng/fboss/bgp/if/bgp_attr.thrift"
include "fboss/routing/policy/if/policy_thrift.thrift"
include "configerator/structs/neteng/bgp_policy/thrift/routing_policy.thrift"
include "thrift/annotation/hack.thrift"
include "thrift/annotation/cpp.thrift"
include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

namespace php fboss
namespace py neteng.fboss.bgp_thrift
namespace py.asyncio neteng.fboss.asyncio.bgp_thrift
namespace py3 neteng.fboss
namespace cpp2 facebook.neteng.fboss.bgp.thrift
namespace go neteng.fboss.bgp_thrift

/**
 * BGP state machine
 */
enum TBgpPeerState {
  IDLE = 0,
  CONNECT = 1,
  ACTIVE = 2,
  OPEN_SENT = 3,
  OPEN_CONFIRMED = 4,
  ESTABLISHED = 5,
  CLOSING = 6,
  IDLE_ADMIN = 7,
}

/*
 * Events in BGP Initialization Process.
 */
enum BgpInitializationEvent {
  /*
   * BGP Initialization process starts
   */
  INITIALIZING = 0,

  /*
   * Platform agent is ready to accept route programming thrift request
   */
  AGENT_CONFIGURED = 1,

  /*
   * Subscribed to FSDB for link status update
   */
  FSDB_SUBSCRIBED = 2,

  /*
   * All peer information has been loaded to establish BGP sessions
   */
  PEER_INFO_LOADED = 3,

  /*
   * All EoR(End-of-Rib) signal have been received for initial RIB computation
   */
  ALL_EOR_RECEIVED = 4,

  /*
   * EoR timer expired to force initial RIB computation
   */
  EOR_TIMER_EXPIRED = 5,

  /*
   * Initial RIB(Routing Information Base) computation has completed
   */
  RIB_COMPUTED = 6,

  /*
   * Initial FIB(Forwarding Information Base) programming based on RIB
   * computation has completed
   */
  FIB_SYNCED = 7,

  /*
   * BGP finished initial FIB sync and advertised best-path selection result
   */
  EOR_SENT = 8,

  /*
   * BGP Initialization process has completed
   */
  INITIALIZED = 9,
}

/**
 * Internal representation of Extended Community (RFC 4360), i.e., this is not
 * what we send or receive on the wire for BGP protocol, but only what we send
 * and receive between applications.
 * We represent our data as union.  For now, the only member of the union is
 * the Two-Byte Asn Extended Community.  We will add more definitions as
 * needed.
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

struct TGoldenPrefixesPolicyStatus {
  1: rib_policy.TRouteFilterPolicy policy;
  2: bool isPolicyActive;
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
  // deprecated (do not use multi_exit_disc field use med field instead)
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
}

/**
 * This is the locally announced BGP network:
 */
struct TBgpNetwork {
  1: bgp_attr.TIpPrefix prefix;
  2: bgp_attr.TAsPath as_path;
  3: list<bgp_attr.TBgpCommunity> communities;
  4: i32 local_pref;
  5: string next_hop4;
  6: string next_hop6;
  7: optional list<TBgpExtCommunity> extCommunities;
}

/**
 * Key parameters for a BGP session. Here peer_id is some
 * unique opaque identifier for a peer, e.g. IPv4 address string
 */
struct TBgpPeer {
  1: string peer_id;
  /* To be deprecated: use i64 version instead */
  @thrift.DeprecatedUnvalidatedAnnotations{items = {"deprecated": "1"}}
  2: i32 local_as;
  @thrift.DeprecatedUnvalidatedAnnotations{items = {"deprecated": "1"}}
  3: i32 remote_as;
  4: i32 hold_time;
  /** only duplex peer has those two, as it may announce routes */
  5: bgp_attr.TIpPrefix next_hop4;
  6: bgp_attr.TIpPrefix next_hop6;
  7: TBgpPeerState peer_state;
  8: bool graceful; // we sent/received GR Capability to/from peer
  9: i64 lastResetHoldTimer; // time is in millisecond
  10: i64 lastSentKeepAlive; // time is in millisecond
  11: i64 lastRcvdKeepAlive; // time is in millisecond
  12: optional bgp_attr.AddPath add_path; // Add path mode if enabled

  // RFC 6793
  13: i64 local_as_4_byte; // unsigned int 32
  14: i64 remote_as_4_byte; // unsigned int 32
  15: i64 lastResetKeepAliveTimer; // time is in millisecond
}

/**
 * Enum for three types of connections
 */
enum TBgpSessionConnectMode {
  PASSIVE_ACTIVE = 1,
  PASSIVE_ONLY = 2,
  ACTIVE_ONLY = 3,
}

/**
 * Negotiated AddPathCapability
 */
struct TBgpAddPathNegotiated {
  1: bgp_attr.TBgpAfi afi;
  2: bgp_attr.AddPath add_path;
}

/**
 * Additional information provided in bgp neighbor command
 */
struct TBgpSessionDetail {
  1: bool confed_peer;
  2: i64 remote_bgp_id; // i64 used to store uint32
  5: bool rr_client;
  6: TBgpSessionConnectMode connect_mode;
  7: i32 peer_port; // i32 used to store uint16
  8: string local_router_id;
  9: i32 local_port; // i32 used to store uint16
  10: bool ipv4_unicast;
  11: bool ipv6_unicast;
  12: i64 gr_restart_time;
  13: i64 gr_remote_restart_time;
  14: i64 eor_sent_time; // time since epoch in milliseconds
  15: i64 eor_received_time; // time since epoch in milliseconds
  16: list<TBgpAddPathNegotiated> add_path_capabilities;
  17: i64 num_of_flaps; // number of terminations for TCP sessions
  18: optional i64 active_time; // time since the socket has been created (BGP ACTIVE state)
  21: i64 sent_update_announcements_ipv4;
  22: i64 sent_update_announcements_ipv6;
  23: i64 recv_update_announcements_ipv4;
  24: i64 recv_update_announcements_ipv6;
  25: i64 sent_update_withdrawals;
  26: i64 recv_update_withdrawals;
  27: i64 enforce_first_as_rejects;
}

/**
 * BGP Session - Outlines transport, peer, capabilities, and statistics
 */
struct TBgpSession {
  1: TBgpPeer peer;
  2: string my_addr;
  3: string peer_addr;
  4: i16 rcvd_prefix_count;
  5: i16 sent_prefix_count;
  6: string description;
  8: i64 uptime;
  /* uptime in msecs */
  9: optional TBgpSessionDetail details;
  10: i64 prepolicy_rcvd_prefix_count;
  11: i64 postpolicy_sent_prefix_count;
  // BGP ID in IPv4 format
  12: string peer_bgp_id;

  # UCMP configurations for this BGP peer
  13: bgp_attr.AdvertiseLinkBandwidth advertise_link_bandwidth;
  14: bgp_attr.ReceiveLinkBandwidth receive_link_bandwidth;
  # This is a typo. Here bandwidth is in Bytes/sec not Bits/sec
  15: optional float link_bandwidth_bps;
  16: i64 reset_time;
  17: i64 num_resets;
  18: string last_reset_reason;
  19: i64 sent_update_msgs;
  20: i64 recv_update_msgs;
  /*
   * Cached RIB version - the max version this peer has consumed from RIB.
   * Used for backpressure visibility to see how caught up this peer is
   * with the current RIB state. A value of 0 indicates the peer is new
   * or down (displayed as "N/A" in CLI).
   */
  21: optional i64 rib_version;
}

struct TPeerEgressStats {
  /*
   * Used to calculate percentile statistics over peers according to the desired
   * grouping. The default grouping is a peer's AdjRibOutGroup.
   */
  1: optional string group_name;
  /*
   * Using these specific session fields from TBgpSession:
   *   - peer id
   *   - sent prefixes
   *   - rcvd prefixes
   *   - sent update messages
   *   - rcvd update messages
   *   - peering state
   *   - peer description
   */
  2: optional TBgpSession session;
  /*
   * Number of transient route updates suppressed; i.e. processed
   * without being advertised in AdjRibOut.
   */
  3: optional i64 transient_route_updates_suppressed;
  /* Number of times adjRibOutQueue_ blocked on a push attempt. */
  4: optional i64 adjribout_queue_blocks;
  /* Total adjRibOutQueue_ block duration. */
  5: optional i64 adjribout_queue_total_block_duration;
  /* Number of times sendQueue_ blocked on a push attempt. */
  6: optional i64 send_queue_blocks;
  /* Total sendQueue_ block duration. */
  7: optional i64 send_queue_total_block_duration;
  /* Total writes buffered in AsyncSocket (i.e. socket backpressured). */
  8: optional i64 total_async_socket_buffered;
  /* Last epoch time (ms) that adjRibOutQueue_ blocked. */
  9: optional i64 last_adjribout_queue_block_time;
  /* Last epoch time (ms) that sendQueue_ blocked. */
  10: optional i64 last_send_queue_block_time;
  /* Last epoch time (ms) that AsyncSocket buffered a message. */
  11: optional i64 last_socket_buffered_time;
}

/**
 * BGP Stream Session attributes
 */
struct TBgpStreamSession {
  1: string subscriber_name;
  // subscription time in msecs
  2: i64 uptime;
  // Remote BGP Peer ID
  3: i32 peer_id;
  // Peer address in IPv4 format
  4: string peer_addr;
  // Number of advertised routes
  5: i64 sent_prefix_count;
  // State of the session
  6: TBgpPeerState state;
  // Number of times session was brought down
  7: i64 num_flaps;
}

/**
 * Local BGP Config information
 */
struct TBgpLocalConfig {
  1: i64 my_router_id; // In network byte order
  /* To be deprecated, use 4 byte version instead */
  @thrift.DeprecatedUnvalidatedAnnotations{items = {"deprecated": "1"}}
  2: i32 local_as;
  @thrift.DeprecatedUnvalidatedAnnotations{items = {"deprecated": "1"}}
  3: optional i32 local_confed_as;

  # UCMP configurations for route programming
  4: bool program_ucmp_weights;
  5: i32 ucmp_width;

  /* RFC 6793 */
  6: i64 local_as_4_byte; // unsigned int32
  7: i64 local_confed_as_4_byte; // unsigned int32

  8: bool enable_update_group;
}

/**
 * Nexthop information for a prefix
 */
struct TNexthopInfo {
  1: bgp_attr.TIpPrefix next_hop;
  2: bool is_reachable;
  3: optional i32 igp_cost;
}

/**
 * Originated route along with host information that we are querying against
 */
struct TOriginatedRouteWithHost {
  1: list<TOriginatedRoute> tOriginatedRoutes;
  2: string host;
  3: string ip;
  4: string oobName;
}

/**
* Locally originated routes
*/
struct TOriginatedRoute {
  1: bgp_attr.TIpPrefix prefix;
  // Deprecated:
  //    Instead newer API returns the path which is associated with
  //    attributes containing local-pref, community, origin code etc.
  //    For backward compatibility with existing CLIs communities will
  //    be filled-in, but support will be removed in future.
  2: optional list<bgp_attr.TBgpCommunity> communities;
  3: TBgpPath path;
  4: i32 minimum_supporting_routes = 0;
  5: bool install_to_fib = false;
  6: i32 supporting_route_count = 0;
  7: optional string policy_name;
  8: optional bool require_nexthop_resolution; // only originate if nexthop is resolved
}

struct NetworkPathWithHost {
  1: map<
    bgp_attrTIpPrefix_cpptemplate_stdmap_895,
    list<bgp_thrift.TBgpPath>
  > networkPath;
  2: string host;
  3: string ip;
  4: string oobName;
}

/**
 * Attributes for locally originating routes
 */
struct TBgpAttributes {
  1: list<bgp_attr.TBgpCommunity> communities;
  2: optional bgp_attr.TAsPath as_path;
  3: optional i32 local_pref;
  4: optional list<TBgpExtCommunity> extCommunities;
  5: optional i32 origin;
  6: optional bgp_attr.TIpPrefix nexthop;
  7: optional bool install_to_fib;
}

/**
* Direction filter for attribute statistics
*/
enum TDirectionFilter {
  INGRESS = 0,
  EGRESS = 1,
  BOTH = 2,
}

/**
* Policy stage filter for attribute statistics
*/
enum TPolicyStageFilter {
  PRE_POLICY = 0,
  POST_POLICY = 1,
  BOTH = 2,
}

/**
* Get Attribute memory statistics
*/
struct TAttributeStats {
  1: i64 total_num_of_attributes;
  2: i64 total_unique_attributes;
  3: double avg_attribute_refcount;
  4: double avg_community_list_len;
  5: double avg_extcommunity_list_len;
  6: double avg_as_path_len;
  7: double avg_cluster_list_len;
  8: double avg_topology_info_len;
}

/**
* Filter parameters for attribute statistics
*/
struct TAttributeStatsFilter {
  1: TDirectionFilter direction = TDirectionFilter.BOTH;
  2: TPolicyStageFilter policyStage = TPolicyStageFilter.BOTH;
}

/**
* Get Entry memory statistics
*/
struct TEntryStats {
  1: i64 total_ucast_routes;
  2: i64 total_rib_paths;
  3: i64 total_adj_ribs;
  4: i64 total_originated_routes;
  5: i64 total_shadow_rib_entries;
  6: i64 total_netlink_wrapper_interfaces;
}

/**
 * BGP Profiler Statistics
 *
 * Lightweight coroutine profiler stats for performance monitoring.
 * Tracks execution time of critical BGP operations.
 */
struct TBgpProfilerStat {
  1: string name;
  2: i64 count;
  3: i64 p50_ms;
  4: i64 p90_ms;
  5: i64 p99_ms;
  6: i64 max_ms;
  7: i64 total_ms;
}

/**
 * Support debug level for BGP.
 */
enum TBgpDebugLevel {
  DEBUG0 = 0,
  DEBUG1 = 1,
  DEBUG2 = 2,
  DEBUG3 = 3,
  DEBUG4 = 4,
  DEBUG5 = 5,
  INFO = 6,
}

/**
 * Result codes for BGP policy change operations
 */
enum BgpPolicyChangeResult {
  POLICIES_APPLIED = 0,
  INTERNAL_ERROR = 1,
  INPUT_ERROR = 2,
  NOT_IMPLEMENTED = 3,
}

/**
* Standard Return Value with success and error message
*/
struct TResult {
  1: bool success = false;
  2: string err = "";
}

// Disk persistence format for BGP Rib to store Rib Policy
struct TRibPolicyStore {
  // Time in seconds at when the data was stored
  1: i64 storedTime;
  // rib policy data
  3: rib_policy.TRibPolicy policy;
  // File termination string to ensure proper truncation of file
  // Sanity check whether data storing completed properly
  100: string fileTermination;
}

/**
 * Drain state struct from BGP which includes
 *  - Node drain state
 *  - Interface drain state
 */
struct TBgpDrainState {
  1: optional bgp_policy.DrainState drain_state;
  2: optional list<string> drained_interfaces;
}

service TBgpService extends fb303.FacebookService {
  /**
   * [Logging]
   *
   * Dynamically set log level for BGP
   *
   * @param levelString: string representation of log level
   *                     (e.g. ".=DBG1,foo.bar=INFO")
   */
  void setLogLevel(1: string levelString);

  /**
   * [Config]
   *
   * Get running config
   */
  string getRunningConfig();

  /**
   * [Config]
   *
   * Same as getRunningConfig, but returns a Thrift struct
   */
  bgp_config.BgpConfig getRunningConfigStruct();

  /**
   * Has policy config artifact
   */
  bool hasPolicySymlink();

  /**
   * [Config]
   *
   * Get policy config
   */
  string getPolicyConfig();

  /**
   * [Config]
   *
   * Validate given config
   * @return <true, ''> on validate config
   *         <false, 'Error Msg'> on invalidate config
   */
  TResult validateConfig(1: string file_name);

  /**
   * [Config]
   *
   * Validate given config and policy as separate artifacts
   * @return <true, ''> on validate config
   *         <false, 'Error Msg'> on invalidate config
   */
  TResult validateConfigAndPolicy(
    1: string config_file_name,
    2: string policy_file_name,
  );

  /**
   * [Config]
   *
   * Fetch and return drain state inside config
   * @return DrainState defined inside bgp_policy.thrift
   */
  TBgpDrainState getDrainState();

  /**
   * [Initialization]
   *
   * Check if BGP++ mark itself converged.
   * @return true when BGP++ marked itself INITIALIZED;
   *         false when BGP++ yet finished initialization sequence;
   */
  bool initializationConverged();

  /**
   * [Initialization]
   *
   * Fetch the initialization event and its corresponding time duration.
   * @return a map of BgpInitializationEvent -> timeDuration mapping.
   */
  map_bgp_thriftBgpInitializationEvent_i64_cpptemplate_stdunordered_map_895 getInitializationEvents();

  /**
   * [Initialization]
   *
   * Return the delta between now and the timestamp of last programmed routes.
   * @return the delta in SECONDS.
   * ATTN: if NO fib programmed routes, return a negative number instead.
   */
  i64 getTimeElapsedSinceLastFibUpdate();

  /**
   * Get the current RIB version. This is a monotonically increasing counter
   * that increments whenever a material routing change occurs (best path
   * or multipath changes). Used for tracking how caught up each peer is
   * with RIB state (backpressure visibility).
   */
  i64 getRibVersion();

  /*
   * Get locally originated routes
   */
  list<TOriginatedRoute> getOriginatedRoutes();

  /*
   * Get route distribution table
   */
  map<string, map<string, bool>> getRouteDistributionTable();

  /**
   * Get a list of active sessions
   */
  list<TBgpSession> getBgpSessions();

  /**
   * Get a list of active stream sessions
   */
  list<TBgpStreamSession> getBgpStreamSessions();

  /**
   * Get deatiled info on list of sessions by peer
   */
  list<TBgpSession> getBgpNeighbors(1: list<string> peer);

  /**
   * Get detailed info on peer egress backpressure statistics.
   */
  list<TPeerEgressStats> getPeerEgressStats();

  /**
   * Get local config information
   */
  TBgpLocalConfig getBgpLocalConfig();

  /**
   * Routes we receive from peer, before policy application
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, TBgpPath> getPrefilterReceivedNetworks(
    1: string peer,
  );

  /**
   * Routes we receive from peer, before policy application with add path
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, list<TBgpPath>> getPrefilterReceivedNetworks2(
    1: string peer,
  );

  /**
   * Routes we receive from one bgp session of a peer, before policy application
   * @param peer: IP address of the peer
   *        sessionBgpId: BGP ID of the session in IPv4 format
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, TBgpPath> getPrefilterReceivedNetworksFromSession(
    1: string peer,
    2: string sessionBgpId,
  );

  /**
   * Routes we receive from one bgp session of a peer, before policy application with add path
   * @param peer: IP address of the peer
   *        sessionBgpId: BGP ID of the session in IPv4 format
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<
    bgp_attr.TIpPrefix,
    list<TBgpPath>
  > getPrefilterReceivedNetworksFromSession2(
    1: string peer,
    2: string sessionBgpId,
  );

  /**
   * Routes we receive from peer, after policy application
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, TBgpPath> getPostfilterReceivedNetworks(
    1: string peer,
  );

  /**
   * Routes we receive from peer, after policy application with add path
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, list<TBgpPath>> getPostfilterReceivedNetworks2(
    1: string peer,
  );

  /**
   * Routes we receive from one bgp session of a peer, after policy application
   * @param peer: IP address of the peer
   *        sessionBgpId: BGP ID of the session in IPv4 format
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, TBgpPath> getPostfilterReceivedNetworksFromSession(
    1: string peer,
    2: string sessionBgpId,
  );

  /**
   * Routes we receive from one bgp session of a peer, after policy application with add path
   * @param peer: IP address of the peer
   *        sessionBgpId: BGP ID of the session in IPv4 format
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<
    bgp_attr.TIpPrefix,
    list<TBgpPath>
  > getPostfilterReceivedNetworksFromSession2(
    1: string peer,
    2: string sessionBgpId,
  );

  /**
   * Get stuff we send to peer after policy
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, TBgpPath> getPostfilterAdvertisedNetworks(
    1: string peer,
  );

  /**
   * Get stuff we send to peer after policy with add path
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, list<TBgpPath>> getPostfilterAdvertisedNetworks2(
    1: string peer,
  );

  /**
   * Get stuff we send to peer prior to policy
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, TBgpPath> getPrefilterAdvertisedNetworks(
    1: string peer,
  );

  /**
   * Get stuff we send to peer prior to policy with add path
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, list<TBgpPath>> getPrefilterAdvertisedNetworks2(
    1: string peer,
  );

  /**
   * Routes we receive from peer, after dry run of new policy config.
   * This API does a dry run of policy config on production routes, without
   * effecting production state, traffic. It applies the given policy
   * (which should be on device) on preIn routes of the peer and displays the
   * postIn output if the policy is applied.
   * i.e. Determine the effect of policy without effecting the running state.
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, TBgpPath> getDryRunPostfilterReceivedNetworks(
    1: string peer,
    2: string file_name,
  );

  /**
   * Routes we sent to a peer, after dry run of new policy config.
   * This API does a dry run of policy config on production routes, without
   * effecting production state, traffic. It applies the given policy
   * (which should be on device) on preOut routes of the peer and displays the
   * postOut output if the policy is applied.
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, TBgpPath> getDryRunPostfilterAdvertisedNetworks(
    1: string peer,
    2: string file_name,
  );

  /**
   * Get post-policy network information for stream subscribers
   *
   * @param peerID - integer ID of BGP stream subscriber
   * @param policy-type - must be either "pre-policy" or "post-policy"
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, list<TBgpPath>> getSubscriberNetworkInfo(
    1: i32 peerID,
    2: string policy_type,
  );

  /**
   * Announce network, afi inferred from prefix. This does not create FIB entry.
   *
   * @param prefix - The prefix to announce
   * @param communities - The list of communities to attach to prefix
   */
  void addNetwork(
    1: bgp_attr.TIpPrefix prefix,
    2: list<bgp_attr.TBgpCommunity> communities,
  );

  /**
   * Remove a previously advertised network. This does not clear FIB entry.
   *
   * @param prefix - The prefix to remove
   */
  void delNetwork(1: bgp_attr.TIpPrefix prefix);

  /**
   * Announce networks, afi inferred from prefix.
   * This does not create FIB entry.
   *
   * @param networks (prefix, attributes) to announce
   */
  @hack.SkipCodegen{reason = "Invalid argument"}
  void addNetworks(1: map<bgp_attr.TIpPrefix, TBgpAttributes> networks);

  /**
   * Remove a previously advertised network.
   * This does not clear FIB entry.
   *
   * @param prefixes - The prefixes to remove
   */
  @hack.SkipCodegen{reason = "Invalid argument"}
  void delNetworks(1: set<bgp_attr.TIpPrefix> prefixes);

  /**
   * Shutdown a peer session
   *
   * @param peer - the peer ip address
   */
  void shutdownSession(1: string peer);

  /**
   * Restart a peer session
   *
   * @param peer - the peer ip address
   */
  void restartSession(1: string peer);

  /**
   * Start a peer session
   *
   * @param peer - the peer ip address
   */
  void startSession(1: string peer);

  /**
   * Dump the current BGP RIB (prefixes learned from others)
   *
   * @param afi - The afi to dump RIB for
   */
  list<TRibEntry> getRibEntries(1: bgp_attr.TBgpAfi afi);

  /**
   * Dump the current Shadow RIB (prefixes learned from the RIB)
   *
   * Though both getRibEntries and getShadowRibEntries provide so
   * called list of TRibEntry. This list differs in interpreation
   *
   * getRibEntries provides a prefix with a set of paths that
   * include both a group of best equal cost paths and a group
   * of other higher cost (not in the group of best). One of
   * the paths from the group of "best" is marked as selected
   * the best path for non-add-path peers
   *
   * getShadowRibEntries contains only group of best equal cost
   * paths to be advertised. And the representation of it when
   * returned through this API will show selected "best" path
   * differetiating from the group of all the best equal cost
   * paths ("multiPaths")
   *
   * @param afi - The afi to dump Shadow RIB for
   */
  list<TRibEntry> getShadowRibEntries(1: bgp_attr.TBgpAfi afi);

  list<TRibEntry> getChangeListEntries(1: bgp_attr.TBgpAfi afi);

  /**
   * Dump the current BGP RIB (prefixes learned from others)
   *
   * @param string - The string representation of the prefix to get
   */
  list<TRibEntry> getRibPrefix(1: string prefix);

  /**
   * Fetch the routes in bgp-local-rib matching the passed in commuinity.
   * Note that *ONLY* paths matching community of a route-entry in
   * the local-rib are returned, i.e. route's paths which does not match
   * passed community are filtered out (even though those might be part of
   * bestpath or ecmp/ucmp paths).
   *
   * @param afi - ipv4 or ipv6 or both
   *
   * @param string - The string represents ASN:NN (both 16bits) value or
   *                 an integer (32bits) representing community.
   *                 Param can also represnet a well-known community from:
   *                 internet | no-advertise | no-export | no-export-subconfed
   */
  list<TRibEntry> getRibEntriesForCommunity(
    1: bgp_attr.TBgpAfi afi,
    2: string community_id,
  );

  /**
   * Fetch the routes in bgp-local-rib matching the passed in commuinity.
   * Note that *ONLY* paths matching community of a route-entry in
   * the local-rib are returned, i.e. route's paths which does not match
   * passed community are filtered out (even though those might be part of
   * bestpath or ecmp/ucmp paths).
   * @param afi - ipv4 or ipv6 or both
   *
   * @param list<string> - The string represents ASN:NN (both 16bits) value or
   *                       an integer (32bits) representing community.
   *                       Param can also represnet a well-known community from:
   *                       internet | no-advertise | no-export | no-export-subconfed
   *
   * community_ids applies match-any logic (in contrast of match-all)
   * e.g. rib entry in returned list should match at least 1 item in community_ids.
   * when community_ids is emtpy, no rib entry should be returned.
   */
  list<TRibEntry> getRibEntriesForCommunities(
    1: bgp_attr.TBgpAfi afi,
    2: list<string> community_ids,
  );

  /*
   * Dump the current BGP RIB whose peer address is in the subnet indicated
   * by a given prefix
   *
   * @param prefix - The string representation of the prefix
   */
  list<TRibEntry> getRibSubprefixes(1: string prefix);

  /**
   * Get RibPolicy.
   *
   * @return thrift struct of TRibPolicy
   */
  rib_policy.TRibPolicy getRibPolicy();

  /**
   * Clear RibPolicy.
   *
   */
  void clearRibPolicy();

  /**
   * [Route Attribute Policy]
   */
  /**
   * Set RouteAttributePolicy.
   *
   * @param policy - The route attribute policy user wants to set with
   * @return <true, ''> on route attribute policy setting success
   *         <false, 'Error Msg'> on route attribute policy setting failure
   */
  TResult setRouteAttributePolicy(1: rib_policy.TRouteAttributePolicy policy);

  /**
   * Get RouteAttributePolicy.
   *
   * @return thrift struct of TRouteAttributePolicy
   */
  rib_policy.TRouteAttributePolicy getRouteAttributePolicy();

  /**
   * Clear RouteAttributePolicy.
   */
  void clearRouteAttributePolicy();

  /**
   * [Path Selection Policy]
   */
  /**
   * Set PathSelectionPolicy.
   *
   * @param policy - The path selection policy user wants to set with
   * @return <true, ''> on path selection policy setting success
   *         <false, 'Error Msg'> on path selection policy setting failure
   */
  TResult setPathSelectionPolicy(1: rib_policy.TPathSelectionPolicy policy);

  /**
   * Get PathSelectionPolicy.
   *
   * @return thrift struct of TPathSelectionPolicy
   */
  rib_policy.TPathSelectionPolicy getPathSelectionPolicy();

  /**
   * Clear PathSelectionPolicy.
   */
  void clearPathSelectionPolicy();

  /**
   * Get the active path selection criteria for the given prefixes.
   *
   * @return a list of TPathSelector which contains the path selection
   *   criteria that is active for the corresponding prefix, i.e., it has
   *   at most one TPathSelectionCriteria in criteria_list. If it is empty,
   *   and bgp_native_path_selection_min_nexthop is not set, the prefix
   *   either does not exist or applies default multipathSelector
   */
  list<rib_policy.TPathSelector> getActivePathSelectionCriteria(
    1: list<string> prefixes,
  );

  /**
   * [Route Filter Policy]
   */
  /**
   * Set RouteFilterPolicy.
   *
   * @param policy - The route filter policy user wants to set with
   * @return <true, ''> on route filter policy setting success
   *         <false, 'Error Msg'> on route filter policy setting failure
   */
  TResult setRouteFilterPolicy(1: rib_policy.TRouteFilterPolicy policy);

  /**
   * [Route Filter Policy]
   */
  /**
   * Clear Ingress and Egress Route Filter Policy.
   *
   * @param
   * @return <true> on policy clearing success
   *         <false> on policy clearing failure
   */
  void clearIngressEgressRouteFiltersPolicy();

  /**
   * [Route Filter Policy]
   */
  /**
   * Clear Golden Prefixes Policy.
   *
   * @param
   * @return <true, ''> on policy clearing success
   *         <false, 'Error Msg'> on policy clearing failure
   */
  void clearGoldenPrefixesPolicy();

  /**
   * [Route Filter Policy]
   */
  /**
   * Get if device is in safemode.
   *
   * @param
   * @return <true> device is in safe mode
   *         <false> device is not in safe mode
   */
  bool getIsSafeModeOn();

  /**
   * Get total golden VIPs count.
   *
   * @param
   * @return total golden VIPs count
   */
  i64 getGoldenVipsCount();

  /**
   * [Route Filter Policy]
   */
  /**
   * Remove safe mode file.
   *
   * @param
   * @return
   */
  void removeSafeModeFile();

  /**
   * Return the golden prefixes policy and it's status (active or inactive).
   */
  TGoldenPrefixesPolicyStatus getGoldenPrefixesPolicyStatus();

  /**
   * Return a mapping from golden prefix to the number of its unique subnets.
   */
  map<string, i32> getGoldenPrefixSubnetCounts();

  /**
   * Get RouteFilterPolicy.
   *
   * @return thrift struct of TRouteFilterPolicy
   */
  rib_policy.TRouteFilterPolicy getRouteFilterPolicy();

  /**
   * Clear RouteFilterPolicy.
   */
  void clearRouteFilterPolicy();

  /**
   * [Watchdog]
   *
   * @param A list of path to the monitored queues/modules
   *    e.g., peer_manager.queue1 returnes the queue sizes of the specific queue
   *    while peer_manager returns all queue sizes monitored in the module
   *    ab empty list would get all of the modules
   * @return a map from monitored paths to the queue sizes, such as
   *    {'peer_manager.queue1', 3}
   */
  monitored_queue_size_map getMonitoredQueueSizes(1: list<string> paths);

  /**
   * Get attribute memory statistics
   */
  TAttributeStats getAttributeStats();

  /**
   * Get attribute memory statistics filtered by ingress/egress and pre/post policy
   */
  TAttributeStats getAttributeStatsFiltered(1: TAttributeStatsFilter filter);

  /**
   * Get entry memory statistics
   */
  TEntryStats getEntryStats();

  /**
   * Get bgp (ingress/egress) policy statistics.
   *
   * @return thrift struct of TPolicyStats
   */
  policy_thrift.TPolicyStats getPolicyStats();

  /**
   * Change log level dynamicaly.
   * For now we support only root-category (recursive) log-level setting.
   * TODO: Allow for per-category log level alteration in future.
   */
  void setDebugLevel(1: TBgpDebugLevel level);

  /**
   * Get routes announced to a peer for specific prefixes  (post-policy)
   *
   * @param peerId: peer that we are asking about
   * @param pfxs: prefixes that we care about
   * @returns: map of prefix -> networks for each of the given prefixes
   * that are announced to this peer.
   */
  @hack.SkipCodegen{reason = "Invalid return type"}
  map<bgp_attr.TIpPrefix, TBgpNetwork> getAdvertisedNetworksFiltered(
    1: string peerId,
    2: list<bgp_attr.TIpPrefix> pfxs,
  );

  /**
   * Get network we receive from peer (pre-policy)
   *
   * @param peerId: peer that we are asking about
   * @returns: map of "route sources" to list of networks for each source
   */
  list<TBgpNetwork> getReceivedNetworks(1: string peerId);

  /**
   * Get network we announce to peer (post-policy)
   *
   * @param peerId: peer that we are asking about
   * @returns: list ot networks
   */
  list<TBgpNetwork> getAdvertisedNetworks(1: string peerId);

  /**
   * Get sessions details for a particular peer and established session
   *
   * @param peerID: peer that we are asking about
   * @param sessionBgpID: specific session for the peer that we are asking about
   * @returns: list of active sessions
  */
  list<TBgpSession> getBgpNeighborsFromSession(
    1: string peerId,
    2: string sessionBgpId,
  );

  /**
   * Set BGP policy for specific peers
   *
   * @param peersPolicy: Map of peer IDs to direction-specific policy names
   * @returns: Result indicating success/failure status
   */
  BgpPolicyChangeResult setPeersPolicy(
    1: map<string, map<bgp_policy.DIRECTION, string>> peersPolicy,
  );

  /**
   * Set BGP policy for peer groups
   *
   * @param peerGroupsPolicy: Map of peer group IDs to direction-specific policy names
   * @returns: Result indicating success/failure status
   */
  BgpPolicyChangeResult setPeerGroupsPolicy(
    1: map<string, map<bgp_policy.DIRECTION, string>> peerGroupsPolicy,
  );

  /**
   * Get nexthop information for a specific prefix
   *
   * @param prefix: The prefix to query nexthop information for
   * @returns: NexthopInfo containing nexthop details for the prefix
   */
  TNexthopInfo getNexthopInfoForNexthop(1: string prefix);

  /**
   * [Profiler]
   *
   * Start/Stop BGP Profiler
   * @param enable - true to start, false to stop
   */
  void startProfiler(1: bool enable);

  /**
   * [Profiler]
   *
   * Set regex filter for BGP Profiler
   * Only functions matching the regex will be profiled.
   * Pass empty string to clear the filter.
   * @param regex - regex string to filter function names
   */
  void setProfilerFilter(1: string regex);

  /**
   * [Profiler]
   *
   * Get BGP Profiler Stats
   * @returns: List of profiler statistics for each tracked function
   */
  list<TBgpProfilerStat> getProfilerStats();

  /**
   * [Profiler]
   *
   * Clear BGP Profiler Stats
   * Resets all counters and histograms.
   */
  void clearProfilerStats();
}

// The following were automatically generated and may benefit from renaming.
typedef bgp_attr.TIpPrefix bgp_attrTIpPrefix_cpptemplate_stdmap_895

@cpp.Type{template = "std::unordered_map"}
typedef map<
  bgp_thrift.BgpInitializationEvent,
  i64
> map_bgp_thriftBgpInitializationEvent_i64_cpptemplate_stdunordered_map_895

@cpp.Type{template = "folly::F14FastMap"}
typedef map<string, i32> monitored_queue_size_map
