cpp_include "folly/container/F14Map.h"

include "common/fb303/if/fb303.thrift"
include "configerator/structs/neteng/bgp_policy/thrift/rib_policy.thrift"
include "configerator/structs/neteng/bgp_policy/thrift/bgp_policy.thrift"
include "configerator/structs/neteng/fboss/bgp/bgp_config.thrift"
include "configerator/structs/neteng/fboss/bgp/if/bgp_attr.thrift"
include "neteng/fboss/bgp/if/policy_thrift.thrift"
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
  /*
   * If the route's weights are overridden by CTE (route attribute policy),
   * the corresponding active UCMP action is set here
   */
  8: optional rib_policy.TRouteAttributeUcmpAction active_cte_ucmp_action;
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
  28: i64 prepolicy_rcvd_prefix_count_ipv4;
  29: i64 prepolicy_rcvd_prefix_count_ipv6;
  30: i64 postpolicy_rcvd_prefix_count_ipv4;
  31: i64 postpolicy_rcvd_prefix_count_ipv6;
  32: i64 prepolicy_sent_prefix_count_ipv4;
  33: i64 prepolicy_sent_prefix_count_ipv6;
  34: i64 postpolicy_sent_prefix_count_ipv4;
  35: i64 postpolicy_sent_prefix_count_ipv6;
  /**
   * TTL Security / GTSM (RFC 5082) status.
   * ttl_security_enabled is derived from whether ttlSecurityHops is configured.
   */
  36: optional bool ttl_security_enabled;
  37: optional i32 ttl_security_hops;
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
  // Policy names configured for this BGP session
  22: optional string ingress_policy_name;
  23: optional string egress_policy_name;
  // Update-group ID assigned by UpdateGroupManager (monotonically increasing)
  24: optional i64 update_group_id;
  // Peer's state in the update-group state machine (e.g., JOINED_RUNNING)
  25: optional string peer_state_update_group;
  26: i64 postpolicy_rcvd_prefix_count;
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
 * Thrift representation of UpdateGroupKey — the criteria for grouping
 * peers into an update group. Mirrors AdjRibStructs.h UpdateGroupKey.
 */
struct TUpdateGroupKey {
  /* Egress policy name applied to peers in this group. */
  1: string egress_policy_name;

  /* Peer device regex or peer-group name for route filtering. */
  2: string route_filter_stmt_name;

  /* Outbound delay in seconds before sending updates. */
  3: i64 out_delay_seconds;

  /* Session type: "EBGP", "IBGP", or "ConfedEBGP". */
  4: string session_type;

  /* Whether IPv4 AFI is negotiated for this group. */
  5: bool afi_ipv4_negotiated;

  /* Whether IPv6 AFI is negotiated for this group. */
  6: bool afi_ipv6_negotiated;

  /* Whether peers in this group are confederation peers. */
  7: bool is_confed_peer;

  /* Whether peers in this group are route reflector clients. */
  8: bool is_rr_client;

  /* Link bandwidth advertisement mode. */
  9: optional bgp_attr.AdvertiseLinkBandwidth advertise_link_bandwidth;

  /* Link bandwidth receive mode. */
  10: optional bgp_attr.ReceiveLinkBandwidth receive_link_bandwidth;

  /* Static link bandwidth in bps for UCMP. Null = use ECMP, zero = blackhole. */
  11: optional i64 link_bandwidth_bps;

  /* Whether private AS numbers are stripped before advertising. */
  12: bool remove_private_asn;

  /* Whether Add-Path send is enabled for this group. */
  13: bool send_add_path;

  /* Whether 4-byte ASN capability is negotiated. */
  14: bool as4_byte_capable;

  /* Whether RFC5549 extended nexthop encoding is negotiated. */
  15: bool ext_nh_encoding_capable;

  /* Peer group name this group belongs to. */
  16: string peer_group_name;

  /* Whether peer has per-peer egress policy override. */
  17: bool peer_override;
}

/**
 * Update group accumulated statistics reported per AdjRibOutGroup.
 */
struct TUpdateGroupStats {
  /* Number of times a slow peer was detached from the group. */
  1: i64 slow_peer_detachments;

  /* Number of DFP (Divergence-Free Peer) rejoin events. */
  2: i64 dfp_rejoin_events;

  /* Number of entries corrected during detached peer rejoin entry collapse. */
  3: i64 collapse_entries_corrected;

  /* Number of DSP (Diverged-State Peer) rejoin events. */
  4: i64 dsp_rejoin_events;

  /* Number of times lazy clone was invoked for detached peers. */
  5: i64 lazy_clone_events;

  /* Cumulative count of IPv4 announcement prefixes sent (monotonic counter). */
  6: i64 total_sent_announcements_ipv4;

  /* Cumulative count of IPv6 announcement prefixes sent (monotonic counter). */
  7: i64 total_sent_announcements_ipv6;

  /* Number of IPv4 update messages sent by this group. */
  8: i64 group_update_messages_ipv4;

  /* Number of IPv6 update messages sent by this group. */
  9: i64 group_update_messages_ipv6;

  /* Number of withdrawals sent by this group. */
  10: i64 group_withdrawals;

  /* Total queue wait time (ms) across all sync peers in the group. */
  11: i64 group_total_queue_wait_ms;

  /* Total number of queue blocks across all sync peers in the group. */
  12: i64 group_total_queue_blocks;

  /* Last epoch time (ms) that a queue block occurred in this group; unset if never. */
  13: optional i64 last_group_queue_block_time;

  /* Current point-in-time count of prefixes in post-policy RIB-OUT (gauge). */
  14: i64 post_out_prefix_count;

  /* Current point-in-time count of IPv4 prefixes in post-policy RIB-OUT. */
  15: i64 post_out_prefix_count_ipv4;

  /* Current point-in-time count of IPv6 prefixes in post-policy RIB-OUT. */
  16: i64 post_out_prefix_count_ipv6;
}

/**
 * Per-peer info within an update group.
 */
struct TUpdateGroupPeerInfo {
  /* Peer address (IPv4 or IPv6). */
  1: string peer_addr;

  /* Peer's state in the update-group state machine (e.g., "JOINED_RUNNING"). */
  2: string peer_state;

  /* Peer's bit position within the group bitmap. */
  3: i64 bit_position;

  /* Whether peer is in-sync with the group (sharing group updates). */
  4: bool is_in_sync;

  /* Whether peer is currently TCP-backpressured. */
  5: bool is_blocked;

  /* Whether peer is detached from the group. */
  6: bool is_detached;

  /* "DFP" or "DSP" if detached, unset otherwise. */
  7: optional string detach_type;

  /* RIB version when peer was detached; unset if not detached. */
  8: optional i64 detached_rib_version;

  /* BGP session state. */
  9: TBgpPeerState session_state;

  /* Peer description from config. */
  10: string description;

  /* Peer's remote AS number. */
  11: i64 remote_as;

  /* Last RIB version seen by this peer. */
  12: i64 last_seen_rib_version;

  /* Current per-peer adjribOut queue depth. */
  13: i64 queue_size;

  /* Number of per-peer adjRibEntry keys (prefix count). */
  14: i64 entry_count;

  /* Epoch time (ms) when EoR was sent to this peer; unset if not sent. */
  15: optional i64 eor_sent_time_ms;
}

/**
 * Complete update-group information for CLI display.
 * Organized by mutability with reserved field ranges for extensibility:
 *   1-2:   Identity (immutable)
 *   3-10:  Per-group configuration (static after group creation)
 *   11-20: Runtime state (changes frequently)
 *   21-30: Accumulated stats
 *   31-40: Per-peer membership
 *   41-50: Diagnostics
 *
 * Note: Slow-peer detection config is global (UpdateGroupConfig in
 * bgp_config.thrift), not per-group. It is displayed in the summary view.
 */
struct TUpdateGroupInfo {
  /* ---- Group identity (fields 1-2) ---- */

  /* Monotonically increasing group ID assigned by UpdateGroupManager. */
  1: i64 group_id;

  /* Full grouping criteria that defines this update group. */
  2: TUpdateGroupKey group_key;

  /* ---- Per-group configuration (fields 3-10) ---- */
  /* fields 3-10 reserved for future per-group configuration */

  /* ---- Runtime state (fields 11-20, changes frequently) ---- */

  /* Group state: "UNINITIALIZED", "IDLE", "READY", "WAITING". */
  11: string group_state;

  /* Total number of peers in this group. */
  12: i64 member_count;

  /* Number of peers currently in-sync with the group. */
  13: i64 in_sync_peer_count;

  /* Number of peers currently detached from the group. */
  14: i64 detached_peer_count;

  /* Number of peers currently blocked (TCP backpressure). */
  15: i64 blocked_peer_count;

  /* Last RIB version seen by this group. */
  16: i64 last_seen_rib_version;

  /* fields 17-20 reserved for future runtime state */

  /* ---- Accumulated stats (fields 21-30) ---- */

  /* Per-group accumulated counters. */
  21: TUpdateGroupStats stats;

  /* fields 22-30 reserved for future stats */

  /* ---- Per-peer membership details (fields 31-40) ---- */

  /* List of peers in this group with their individual state. */
  31: list<TUpdateGroupPeerInfo> peers;

  /* fields 32-40 reserved for future per-peer data */

  /* ---- Diagnostics (fields 41-50) ---- */

  /* Epoch time (ms) when initial RIB dump completed for this group. */
  41: optional i64 initial_dump_completion_time_ms;

  /* Total number of entry discrepancies detected during rejoin collapse. */
  42: i64 total_discrepancies;

  /* Per-PeerUpdateState count breakdown (e.g., "JOINED_RUNNING" -> 3). */
  43: map<string, i64> peer_state_counts;
  /* fields 44-50 reserved for future diagnostics */
}

/**
 * Request parameters for getUpdateGroupInfo().
 * All fields are optional — omitting them returns all groups.
 */
struct TGetUpdateGroupInfoRequest {
  /* Filter by a specific update group ID. Unset returns all groups. */
  1: optional i64 group_id;
}

/**
 * Response wrapper for getUpdateGroupInfo().
 * Allows adding response metadata (e.g., pagination, timestamp)
 * without breaking the API signature.
 */
struct TGetUpdateGroupInfoResponse {
  1: list<TUpdateGroupInfo> update_groups;

  /* Whether the update-group feature is enabled. */
  2: bool enable_update_group;
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
  @thrift.AllowUnsafeNonSealedKeyType
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
 * Result codes for BGP config change operations
 */
enum BgpConfigChangeResult {
  CONFIG_APPLIED = 0,
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

/**
 * Hold timer information for a BGP peer.
 * Exposes the remaining hold time before the session would be torn down.
 */
struct THoldTimerInfo {
  /** Peer IP address */
  1: string peer_address;
  /** Remaining hold time in milliseconds before the timer expires.
   *  Returns 0 for peers without an active hold timer or overdue timers. */
  2: i32 hold_time_remaining_ms;
}

/**
/**
 * Outcome of a single health check.
 * Used for per-check status, per-module status, and overall report status.
 * UNKNOWN=0: thrift defaults enum fields to 0 in newly constructed structs.
 *
 * PASS:    Check ran successfully; measured value is within healthy range.
 * WARN:    Check ran; value is degraded but not immediately traffic-impacting.
 * FAIL:    Check is in unhealthy state — either value is outside healthy
 *          range OR the check could not execute (e.g. counter missing).
 * SKIPPED: Check was intentionally not run (e.g. EBB-only check on DC).
 *
 * Precedence (worst wins): FAIL > WARN > PASS > SKIPPED
 */
enum HealthCheckStatus {
  UNKNOWN = 0,
  PASS = 1,
  FAIL = 2,
  SKIPPED = 3,
  WARN = 5,
}

/**
 * Health check category.
 * GLOBAL_* categories are subcategories under the "General BGP Health"
 * section in the RFC doc. Other categories map to individual BGP++ modules.
 * See: https://docs.google.com/document/d/1toZUF79Nx7erIU6RrrOJ12EXeQy7GrYHx-5YX7aIeOk
 */
enum HealthCheckCategory {
  UNKNOWN = 0,
  GLOBAL_SYSTEM = 1,
  GLOBAL_TASK_THREAD = 2,
  GLOBAL_CONVERGENCE = 3,
  SESSION_MANAGER = 4,
  PEER_MANAGER = 5,
  RIB = 6,
  NETLINK_WRAPPER = 7,
  NEXTHOP_TRACKER = 8,
  FIB_AGENT = 9,
  THRIFT_ENDPOINT = 10,
}

/**
 * Unique identifier for each health check.
 * Each category is allocated a range of 100 values for future expansion.
 * See: https://docs.google.com/document/d/1toZUF79Nx7erIU6RrrOJ12EXeQy7GrYHx-5YX7aIeOk
 */
enum HealthCheckId {
  HEALTH_CHECK_UNKNOWN = 0,

  /* GLOBAL_SYSTEM */
  GLOBAL_SYSTEM_THRIFT_REACHABLE = 101,
  GLOBAL_SYSTEM_RSS_MEMORY = 102,
  GLOBAL_SYSTEM_CPU_USAGE = 103,
  GLOBAL_SYSTEM_QUEUE_SIZES = 104,
  GLOBAL_SYSTEM_MEMORY_LEAK = 105,
  GLOBAL_SYSTEM_FD_COUNT = 106,
  GLOBAL_SYSTEM_THREAD_COUNT = 107,
  GLOBAL_SYSTEM_ATTR_DEDUP = 108,
  GLOBAL_SYSTEM_PLANNED_EXIT = 109,

  /* GLOBAL_TASK_THREAD_STATUS */
  GLOBAL_TASK_HEARTBEATS = 201,
  GLOBAL_TASK_UPTIME = 202,
  GLOBAL_TASK_STUCK_TASKS = 203,
  GLOBAL_TASK_WATCHDOG = 204,

  /* GLOBAL_CONVERGENCE */
  GLOBAL_CONVERGENCE_INITIALIZED = 301,
  GLOBAL_CONVERGENCE_TIME = 302,
  GLOBAL_CONVERGENCE_INIT_TIMEOUT = 303,
  GLOBAL_CONVERGENCE_EOR_RECEIVED = 304,
  GLOBAL_CONVERGENCE_INIT_PHASES = 305,
  GLOBAL_CONVERGENCE_SAFE_MODE = 306,
  GLOBAL_CONVERGENCE_STALE_PATHS = 307,
  GLOBAL_CONVERGENCE_STUCK_HANDSHAKE = 308,

  /* SESSION_MANAGER */
  SESSION_PORT_179 = 401,
  SESSION_ESTABLISHED = 402,
  SESSION_ACTIVE_ADJRIBS = 403,
  SESSION_FLAPS = 404,
  SESSION_HOLD_TIMER_EXPIRY = 405,
  SESSION_PEER_QUEUE = 406,
  SESSION_COLLISIONS = 407,
  SESSION_NOTIFICATIONS = 408,
  SESSION_SOCKET_BYTES = 409,
  SESSION_KEEPALIVES = 410,
  SESSION_SOCKET_ERRORS = 411,
  SESSION_TCP_WRITE_BLOCKS = 412,

  /* PEER_MANAGER */
  PEER_ZERO_ROUTES = 501,
  PEER_SLOW_DETACHED = 502,
  PEER_DROPPED_PREFIXES = 503,
  PEER_THRIFT_REJECTS = 504,
  PEER_PREFIX_COUNT = 505,
  PEER_POLICY_CACHE = 506,
  PEER_RIB_VERSION = 507,
  PEER_CHANGELIST_SIZE = 508,
  PEER_CONSUMER_LAG = 509,
  PEER_STUCK_BATONS = 510,

  /* RIB */
  RIB_ORIGINATED_ROUTES = 601,
  RIB_PATH_SELECTION_DURATION = 602,
  RIB_RECEIVED_ROUTES = 603,
  RIB_TABLE_VERSION = 604,
  RIB_ROUTE_CHURN = 605,
  RIB_OVERLOAD_MODE = 606,
  RIB_SHADOW_CONSISTENT = 607,
  RIB_PAUSE_TIME = 608,
  RIB_MODULE_RESPONSIVENESS = 609,

  /* NETLINK_WRAPPER */
  NETLINK_TRACKED_INTERFACES = 701,
  NETLINK_FIB_QUEUE = 702,
  NETLINK_SYNC_STATUS = 703,
  NETLINK_INTERFACE_STABLE = 704,
  NETLINK_SOCKET_CONNECTED = 705,

  /* NEXTHOP_TRACKER */
  NHT_CONFIG_CONSISTENT = 801,
  NHT_STREAM_CONNECTED = 802,
  NHT_UNREACHABLE_NEXTHOPS = 803,
  NHT_FSDB_SUBSCRIPTIONS = 804,
  NHT_CACHE_NOT_EMPTY = 805,

  /* FIB_AGENT */
  FIB_AGENT_CONNECTED = 901,
  FIB_AGENT_SYNCED = 902,
  FIB_AGENT_UPDATE_FAILURES = 903,
  FIB_AGENT_STATUS_FAILURES = 904,
  FIB_AGENT_ROUTES_EXIST = 905,
  FIB_AGENT_LAST_UPDATE_TIME = 906,
  FIB_AGENT_HOLDDOWN = 907,
  FIB_AGENT_FLUSH_EVENTS = 908,

  /* THRIFT_ENDPOINT */
  THRIFT_ALIVE = 1001,
  THRIFT_STREAMING_REJECTS = 1002,
  THRIFT_DRAIN_STATE = 1003,
  THRIFT_ACTIVE_CONNECTIONS = 1004,
}

/**
 * Result of a single health check.
 * Each check is identified by its HealthCheckId enum value.
 */
struct THealthCheckResult {
  /** Unique check identifier (use enumNameSafe() to get readable name) */
  1: HealthCheckId checkId;

  /** Health check category */
  2: HealthCheckCategory category;

  /** Pass/Warning/Fail/Skipped/Error */
  3: HealthCheckStatus status;

  /* Field 4 (severity) removed — ID reserved to prevent reuse */

  /** Human-readable detail with observed values */
  5: string message;

  /** Observed numeric value (when applicable) */
  6: optional double observedValue;

  /** Threshold compared against (when applicable) */
  7: optional double threshold;

  /** Unix epoch ms when the check ran */
  8: i64 timestampMs;
}

/**
 * Health report for a single category.
 */
struct TModuleHealthReport {
  /** Which category */
  1: HealthCheckCategory category;

  /** All check results for this category */
  2: list<THealthCheckResult> checks;

  /** PASS if all checks passed, FAIL if any failed, SKIPPED if all skipped */
  3: HealthCheckStatus overallStatus;
}

/**
 * Top-level health report aggregating all categories.
 * Returned by getHealthReport().
 */
struct THealthReport {
  /** Per-category reports, one per HealthCheckCategory */
  1: list<TModuleHealthReport> modules;

  /** PASS if all checks passed, FAIL if any failed */
  2: HealthCheckStatus overallStatus;

  /** Unix epoch ms when report was generated */
  3: i64 timestampMs;

  /** Summary counts */
  4: i32 passCount;
  5: i32 failCount;
  6: i32 skipCount;
  8: i32 warnCount;
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
   * Get detailed information about all active update groups.
   */
  TGetUpdateGroupInfoResponse getUpdateGroupInfo(
    1: TGetUpdateGroupInfoRequest request,
  );

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
  void addNetworks(
    @thrift.AllowUnsafeNonSealedKeyType
    1: map<bgp_attr.TIpPrefix, TBgpAttributes> networks,
  );

  /**
   * Remove a previously advertised network.
   * This does not clear FIB entry.
   *
   * @param prefixes - The prefixes to remove
   */
  @hack.SkipCodegen{reason = "Invalid argument"}
  void delNetworks(
    @thrift.AllowUnsafeNonSealedKeyType
    1: set<bgp_attr.TIpPrefix> prefixes,
  );

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
   * Unset BGP policy for specific peers, causing them to fall back to their
   * peer group's policy via the policy resolution hierarchy.
   *
   * @param peersToUnset: Map of peer IDs to set of directions to unset
   * @returns: Result indicating success/failure status
   */
  BgpPolicyChangeResult unsetPeersPolicy(
    1: map<string, set<bgp_policy.DIRECTION>> peersToUnset,
  );

  /**
   * Incorporate a collection of BGP peers into the operational configuration.
   * Each peer config may optionally reference an existing peer_group_name
   * to facilitate configuration inheritance from the designated peer group.
   *
   * Currently accepted BgpPeer fields:
   *   - peer_addr
   *   - local_addr
   *   - next_hop4
   *   - next_hop6
   *   - description
   *   - peer_id
   *   - ingress_policy_name
   *   - egress_policy_name
   *   - remote_as_4_byte
   *   - peer_group_name
   * Setting any other field will return BgpConfigChangeResult::INPUT_ERROR.
   *
   * @param peers - A list of BgpPeer configurations to be added
   *                (definition - https://fburl.com/code/ha28hjuf)
   * @returns: A BgpConfigChangeResult object indicating the outcome
   */
  BgpConfigChangeResult addPeers(1: list<bgp_config.BgpPeer> peers);

  /**
   * Remove BGP peers from the operational configuration.
   * Only static peers (non-CIDR) are accepted. Dynamic peers are rejected.
   * Non-existent peer addresses are treated as no-ops.
   *
   * @param peerAddrs - A list of peer address strings (IPv4 or IPv6) to remove
   * @returns: A BgpConfigChangeResult indicating the outcome
   */
  BgpConfigChangeResult delPeers(1: list<string> peerAddrs);

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

  /**
   * [Telemetry]
   *
   * Get the remaining hold timer value for BGP peers.
   * Returns per-peer hold timer info including the remaining time
   * before the hold timer expires.
   *
   * @param peers: list of peer IP addresses to query. If empty, returns
   *               hold timer info for all peers.
   * @returns: list of THoldTimerInfo with remaining hold time per peer.
   */
  list<THoldTimerInfo> getHoldTimers(1: list<string> peers);

  /**
   * [Health]
   *
   * Run all registered health checks and return a comprehensive
   * health report for the BGP++ daemon.
   * Covers internal checks (system resources, task health, convergence,
   * session/peer/rib state) and external checks (FIB agent, thrift
   * endpoint). Platform-aware: EBB-only checks are skipped on DC.
   */
  THealthReport getHealthReport();

  /**
   * [Partial Drain]
   *
   * Get device-level partial-drain status
   */
  TPartialDrainStatus getPartialDrainStatus();

  /**
   * [Partial Drain]
   *
   * Get full partial-drain state including affected prefix list.
   */
  TPartialDrainState getPartialDrainState();

  /**
   * [Partial Drain]
   *
   * Get list of prefixes currently in partial-drain state.
   */
  list<TPartiallyDrainedPrefix> getPartiallyDrainedPrefixes();
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
