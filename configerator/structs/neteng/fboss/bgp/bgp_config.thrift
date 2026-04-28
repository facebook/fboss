/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

namespace cpp2 facebook.bgp.thrift
namespace py neteng.fboss.bgp_config
namespace py3 configerator.structs.neteng.fboss.bgp
namespace go neteng.fboss.bgp_config

include "configerator/structs/neteng/bgp_policy/thrift/bgp_policy.thrift"
include "configerator/structs/neteng/fboss/bgp/if/bgp_attr.thrift"
include "thrift/annotation/thrift.thrift"
include "thrift/annotation/python.thrift"
include "configerator/structs/neteng/config/vip_service_config.thrift"

package "facebook.com/neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/bgp_config"

struct ClassId {
  /*
   * classID is a way in agent to tag packets.
   */
  1: i32 value;
  /*
   * mininmum supporting paths for the prefix in the rib table
   * to allow route programming for the class id
   */
  2: optional i64 minimum_supporting_routes;
}

struct BgpPeerTimers {
  1: i32 hold_time_seconds;
  2: i32 keep_alive_seconds;
  3: i32 out_delay_seconds;
  4: i32 withdraw_unprog_delay_seconds;
  5: optional i32 graceful_restart_seconds;
  6: optional i32 graceful_restart_end_of_rib_seconds;
}

struct RouteLimit {
  // Maximum routes allow for this peer
  1: i64 max_routes = 12000; // default 12000

  // Only emit warning when max route limit exceeded
  2: bool warning_only = false; // default false

  // Number of routes at which warning is to be issued.
  3: i64 warning_limit = 0; // default 0, not using warning_limit
}

struct PeerGroup {
  1: string name;
  2: optional string description;
  /* Basic peer config */
  @thrift.Deprecated
  3: optional i32 remote_as; // To be deprecated, use 4 byte version
  4: optional string local_addr;
  5: optional string next_hop4;
  6: optional string next_hop6;
  7: optional bool enabled;
  8: optional i32 router_port_id;
  9: optional i64 remote_as_4_byte; // unsigned int 32, RFC 6793

  /* BgpPgFeatures in generic_switch_config.thrift */
  10: optional bool is_passive;
  11: optional bool is_confed_peer;
  12: optional bool is_rr_client;
  13: optional bool next_hop_self;
  14: optional bool remove_private_as;

  /* BgpPgMultiprotocolAttrs in generic_switch_config.thrift */
  20: optional bool disable_ipv4_afi;
  21: optional bool disable_ipv6_afi;
  /* Policy name for ingress and egress policies */
  22: optional string ingress_policy_name;
  23: optional string egress_policy_name;
  /* BgpPgTimers in generic_switch_config.thrift */
  24: optional BgpPeerTimers bgp_peer_timers;
  /* This is peer_tag/type in BgpPeer */
  25: optional string peer_tag;
  /* Local-AS# RFC-7705 */
  @thrift.Deprecated
  26: optional i32 local_as; // To be deprecated, use 4 byte version
  37: optional i64 local_as_4_byte; // unsigned int32, RFC 6793
  /* Route caps for pre & post filter prefixes */
  28: optional RouteLimit pre_filter;
  29: optional RouteLimit post_filter;
  /* Enable stateful HA to preserve preIn during restart */
  30: optional bool enable_stateful_ha;

  31: optional bgp_attr.AddPath add_path;
  /* Advertising ipv4 nlri over v6 nexthop # RFC-5549 */
  32: optional bool v4_over_v6_nexthop;

  /**
   * BGP Link-Bandwidth advertisement & receipt knobs
   */
  27: optional bgp_attr.AdvertiseLinkBandwidth advertise_link_bandwidth;
  35: optional bgp_attr.ReceiveLinkBandwidth receive_link_bandwidth;

  /*
   *  Link bandwidth to be advertised in bits (not bytes) per second.  Optional
   *  valid suffixes are K, M, G, for example, "100G"
   */
  33: optional string link_bandwidth_bps;

  /* For redistribute peer, do not program received routes */
  34: optional bool is_redistribute_peer;

  /* Enable enforce_first_as */
  36: optional bool enforce_first_as;

  /**
   * TTL Security / GTSM (Generalized TTL Security Mechanism, RFC 5082).
   * When set, outbound BGP packets use TTL=255 and inbound packets are
   * checked for TTL >= (256 - ttl_security_hops).
   */
  38: optional i32 ttl_security_hops;
}

/**
 * The configuration of a single BGP peer
 */
struct BgpPeer {
  /**
   * the peer's BGP ASN
   */
  @thrift.Deprecated
  1: optional i32 remote_as; // To be deprecated, use 4 byte version
  35: optional i64 remote_as_4_byte; // unsigned int32, RFC 6793
  /**
   * local address to connect/listen on
   */
  2: string local_addr;
  /**
   * remote address to connect to or accept connections from
   * this could be either address or prefix spec (e.g. x.x.x.x/24)
   * the latter only works for passive listening sessions
   */
  3: string peer_addr;
  /**
   * next-hop values to use in outgoing updates
   */
  4: string next_hop4;
  5: string next_hop6;
  /**
   * description for the peer, e.g. csw05a.frc1
   */
  6: optional string description;
  /**
   * passive peer will listen for incoming connections
   * the port to bind to is the listen_port from global config
   * default is to actively connect to the peer
   */
  7: optional bool is_passive;
  /**
   * remote confederation as number
   */
  8: optional bool is_confed_peer;

  /**
   * peer type
   */
  9: optional string type;
  10: optional string peer_id;
  /**
  * whether we should reflect this peer's routes
  */
  11: optional bool is_rr_client;
  /**
  * the tag to group similar peers
  */
  12: optional string peer_tag;
  /**
  * whether we should set next-hop to self to this peer
  * this works for all AFI's.
  */
  13: optional bool next_hop_self;
  /**
  * By default all AFI's are enabled, below allows to disable
  * them selectively. This is done for backward compat with
  * old configs.
  */
  14: optional bool disable_ipv4_afi;
  15: optional bool disable_ipv6_afi;

  /**
   * the port that this peer is found on. This is used to remove peers quickly
   * in case the port goes down.
   */
  18: optional i32 router_port_id;
  19: optional BgpPeerTimers bgp_peer_timers;

  /**
   * Whether we expect the peer to be present. (Not taken by BGP++)
   */
  20: optional bool enabled;

  /**
   * Remove private AS
   */
  21: optional bool remove_private_as;

  /**
   * Policy name for ingress and egress policies.
   * Bgp++ refers policy using name instead of BgpPolicy to avoid replication of
   * policy in the config.
   */
  22: optional string ingress_policy_name;
  23: optional string egress_policy_name;

  /**
   * Local-AS# for this peer. Only valid for eBGP peers.
   * Overrides the global config's local-as for this peer.
   * Reference: RFC-7705
   */
  @thrift.Deprecated
  25: optional i32 local_as; // To be deprecated, use 4 byte version
  37: optional i64 local_as_4_byte; // unsigned int32, RFC 6793

  /* Route caps for pre & post filter prefixes */
  27: optional RouteLimit pre_filter;
  28: optional RouteLimit post_filter;

  /* Enable stateful HA to preserve preIn during restart */
  29: optional bool enable_stateful_ha;

  /* Advertising ipv4 nlri over v6 nexthop # RFC-5549 */
  31: optional bool v4_over_v6_nexthop;

  /**
   * BGP Link-Bandwidth advertisement & receipt knobs
   */
  26: optional bgp_attr.AdvertiseLinkBandwidth advertise_link_bandwidth;
  34: optional bgp_attr.ReceiveLinkBandwidth receive_link_bandwidth;

  /*
   *  Link bandwidth to be advertised in bits (not bytes) per second.  Optional
   *  valid suffixes are K, M, G, for example, "100G"
   */
  32: optional string link_bandwidth_bps;

  /* For redistribute peer, do not program received routes */
  33: optional bool is_redistribute_peer;

  /**
   * Peer Group name. peer config overwrites peer group config
   */
  99: optional string peer_group_name;

  /**
   * Flag to control whether we want to enable add_path
   * functionality to this peer.
   */
  100: optional bgp_attr.AddPath add_path;

  /* Enable enforce_first_as */
  101: optional bool enforce_first_as;

  /**
   * TTL Security / GTSM (Generalized TTL Security Mechanism, RFC 5082).
   * When set, outbound BGP packets use TTL=255 and inbound packets are
   * checked for TTL >= (256 - ttl_security_hops).
   * Typical value: 1 for directly connected eBGP peers.
   */
  102: optional i32 ttl_security_hops;
}

/**
 * The spec of an advertised BGP network
 *
 * Note: When attributes are given along with policy_name,
 *       attributes are set first and then policy is applied
 */
struct BgpNetwork {
  1: string prefix;
  2: optional list<string> communities;
  3: optional i32 minimum_supporting_routes;
  4: optional bool install_to_fib;
  5: optional string policy_name; // policy to modify other attributes
  6: optional i32 local_pref;
  7: optional i32 origin;
  8: optional bgp_attr.TAsPath as_path;
  9: optional string nexthop;
  10: optional bool require_nexthop_resolution; // only originate if nexthop is resolved
}

struct BgpCommunity {
  1: string name;
  2: string description;
  3: list<string> communities;
}

struct BgpLocalPref {
  1: i32 localpref;
  2: string name;
  3: string description;
}

/**
 * BGP UCMP quantizer (non-uniform)
 */
struct BgpUcmpQuantizerConfig {
  /**
   * minimum quantization step in bps, typically chosen as individual link speed
   * e.g "100G"
   */
  1: string min_step_bps;

  /**
   * quantization error = quantized-bps / input-bps - 1
   * error threshold sets upper bound threshold for quantization error for all inputs
   * e.g 0.1 (10%)
   */
  2: double error_pct_threshold;

  /**
   * This list contains bps that's ensured to be part of quantized-bps output, quantizer
   * internall will go through each of them and find rest of quantized-bps with top-down
   * approach
   * This list has to contain the maximum capacity we would like to support
   * e.g ["2400G", "3600G"]
   */
  3: list<string> fixed_quantized_bps_list;
}

/**
 * BGP Policy configuration thrift struct
 */
struct BgpPolicyConfig {
  /**
   * List of community names and descriptions
   */
  1: list<BgpCommunity> communities = [];

  /**
   * List of localpref values with their
   * mnemonic name and description
   */
  2: list<BgpLocalPref> localprefs = [];

  /**
   * Policies used by BGP++
   */
  3: optional bgp_policy.BgpPolicies policies;
}

// Settings for how BGP behaves if switch prefix limits are exceeded.
enum OverloadProtectionMode {
  // After the limit is exceeded, drop all further prefixes.
  DROP_EXCESS_PREFIXES = 1,

  // After the limit is exceeded, apply the golden prefix policy.
  APPLY_GOLDEN_PREFIX_POLICY = 2,
}

/**
 * The switch level limit to enforce to cap the max path/route BGP can support.
 */
struct BgpSwitchLimitConfig {
  /**
   * This is the max-cap of the total received path count from all BGP sessions.
   */
  1: optional i64 ingress_path_limit;

  /**
   * This is the max-cap of the total unique prefixes received from BGP peers.
   */
  2: optional i64 prefix_limit;

  /**
   * This is the max-cap of the total path limit, that includes both ingress and egress paths.
   */
  3: optional i64 total_path_limit;

  4: OverloadProtectionMode overload_protection_mode = DROP_EXCESS_PREFIXES;
  /**
  * upperbound of allowed golden vips per device
  **/
  5: optional i32 max_golden_vips;
}

enum VerifyClientType {
  /**
   * Request a cert and verify it. Fail if verification fails or no cert is presented
   */
  ALWAYS = 0,
  /**
   * Request a cert from the peer and verify if one is presented. Will fail if verification fails. Do not fail if no cert is presented.
   */
  IF_PRESENTED = 1,
  /**
   * No verification is done and no cert is requested.
   */
  DO_NOT_REQUEST = 2,
}

struct ThriftServerConfig {
  /**
   * Flag to enable TLS for the thrift server
   */
  1: bool enable_tls = false;

  /**
   * CA path for verifying peers
   */
  2: optional string x509_ca_path;

  /**
   * Cert path
   */
  3: optional string x509_cert_path;

  /**
   * Key path
   */
  4: optional string x509_key_path;

  /**
   * ECC curve used, will mostly be using prime256v1
   */
  5: optional string ecc_curve_name;

  /**
   * Verification method to use for client certificates
   */
  6: optional VerifyClientType verify_client_type;
}

struct MemoryProfilingConfig {
  /**
  * Flag to enable memory profiling
  */
  1: bool enable_memory_profiling = false;
  2: i32 heap_dump_interval_s = 300;
}

/**
 * Update group configuration for slow peer detection and detachment.
 */
struct UpdateGroupConfig {
  /**
   * Allow slow peer detachment from update groups.
   * When false, slow peers will NOT be detached regardless of thresholds.
   */
  1: bool allowSlowPeerDetach = false;

  /**
   * Duration threshold (ms) for slow peer detection.
   * If a peer is continuously blocked longer than this, it is detached.
   */
  2: i32 slowPeerTimeThresholdMs = 50000;

  /**
   * Number of times a peer can become blocked within the rolling window
   * before being classified as slow and detached.
   */
  3: i32 slowPeerBlockCountThreshold = 10;

  /**
   * Rolling window duration (ms) for frequency-based slow peer detection.
   */
  4: i32 slowPeerBlockCountWindowMs = 1000;

  /**
   * Enable serializing group PDU for update groups.
   * When enabled, the group serializes BGP UPDATE once and distributes
   * to all peers. Each peer only mutates the nexthop.
   */
  5: bool enableSerializeGroupPdu = true;
}

struct BgpSettingConfig {
  @python.DeprecatedSortSetOnSerialize
  1: set<string> features;

  /**
   * Enable BGP Med comparison in path selection
   */
  2: optional bool enable_med_comparison;

  /**
   * Enable BGP Med missing as worst
   */
  3: optional bool enable_med_missing_as_worst;

  /**
   * Enable BGP Weight comparison in path selection
   */
  4: optional bool enable_weight_comparison;

  /**
   * Enable Next Hop Tracking and IGP cost comparision in path selection
   */
  5: optional bool enable_next_hop_tracking;

  /**
   * List of regex patterns for interface name matching in nexthop tracking.
   */
  6: optional list<string> include_interface_regexes;

  /**
   * Enable Dynamic Policy Evaluation for prefix-list and routing policy
   */
  7: optional bool enable_dynamic_policy_evaluation;

  /**
   * Enable egress queue backpressure when queueing updates to TCP socket.
   */
  8: optional bool enable_egress_queue_backpressure;

  /**
   * Enable using path IDs allocated upon selection in Rib for outgoing updates.
   */
  9: optional bool enable_rib_allocated_path_id;

  /**
   * Enable BGP Update Groups for shared update generation
   */
  10: optional bool enable_update_group;

  /**
   * Config to activate memory profiling on non-Meta device
   */
  11: optional MemoryProfilingConfig memory_profiling_config;

  /**
  * Enable BGP++ to use optimized GR logic
  */
  13: optional bool enable_optimized_GR;

  /**
  * Enable eiBGP multipath
  */
  14: optional bool enable_eibgp_multipath;

  /**
   * Configuration for update group slow peer detection and detachment.
   */
  15: optional UpdateGroupConfig update_group_config;
}

/**
 * NetServiceFramework configuration for EBB deployment
 */
struct BgpNetServiceThriftConfig {
  // Service identity for ACL checking
  1: optional string net_service_identity;

  // Path to static ACL JSON file
  2: optional string net_static_file_acl;

  // Kill switch file to disable ACL checking
  3: optional string net_auth_checker_kill_switch_file;

  // TLS/mTLS certificate paths
  4: optional string mtls_cert;
  5: optional string mtls_ca;

  // Network configuration
  6: optional i32 dscp;
  7: optional i32 vrf_creation_timeout_s;

  // Threading configuration
  8: optional i32 thrift_num_io_worker_threads;
  9: optional i32 thrift_num_cpu_worker_threads;

  // FB303 counter configuration
  10: optional bool export_counters_time_series;
  11: optional i32 counters_time_series_duration;

  // Thrift server behavior
  12: optional bool propagate_thrift_default_protocols;
}

/**
 * BGP configuration thrift struct for a switch
 */
struct BgpConfig {
  /**
   * this is normally the loopback IP
   */
  1: string router_id;

  /**
   * local ASN is same for all peers for simplicity
   */
  @thrift.Deprecated
  2: optional i32 local_as; // To be deprecated, use 4 byte version

  /**
   * v4 and v6 networks to announce
   */
  3: list<BgpNetwork> networks4;
  4: list<BgpNetwork> networks6;

  /**
   * the list of BGP peers for the switch
   */
  5: list<BgpPeer> peers;

  /**
   * the hold time for all BGP sessions
   */
  6: i32 hold_time = 30;

  /**
   * the TCP port to listen on for passive sessions
   */
  7: i32 listen_port = 179;

  /**
   * the confederation asn for this router, if any
   */
  @thrift.Deprecated
  8: optional i32 local_confed_as; // To be deprecated, use 4 byte version

  /**
   * Address to listen on, defaults to ::
   */
  10: string listen_addr = "::";

  /**
   * List of community names and descriptions
   */
  11: list<BgpCommunity> communities = [];

  /**
   * List of localpref values with their
   * mnemonic name and description
   */
  12: list<BgpLocalPref> localprefs = [];

  14: optional i32 graceful_restart_convergence_seconds;

  /**
   * Policies used by BGP++
   */
  15: optional bgp_policy.BgpPolicies policies;

  /**
   * the list of BGP peer groups used by peers
   */
  16: optional list<PeerGroup> peer_groups;

  /**
   * Use received link bandwidth extended community to compute UCMP paths
   */
  17: optional bool compute_ucmp_from_link_bandwidth_community;

  18: i32 eor_time_s = 120;

  /**
   * Quantization bucket size for UCMP weights (received link-bandwidth) for
   * programming.
   */
  19: i32 ucmp_width = 65636;

  /**
   * BGP UCMP quantizer
   */
  20: optional BgpUcmpQuantizerConfig ucmp_quantizer_config;

  /**
  * VipService related configs
  */
  21: bool enable_vip_service = false;
  22: optional vip_service_config.VipServiceConfig vip_service_config;

  /**
   * 4-byte ASN definition for local ASN and local confed ASN
   *
   * RFC-6793: BGP Support for Four-Octet Autonomous System (AS) Number Space
   */
  23: optional i64 local_as_4_byte;
  24: optional i64 local_confed_as_4_byte;

  /**
   * Count confed as in as path length
   */
  25: optional bool count_confeds_in_as_path_len;

  /**
   * This is used to override the default command line arguments we
   * pass to the agent.
   */
  26: map<string, string> defaultCommandLineArgs = {};

  /**
  * classID is a way in agent to tag packets.
  */
  27: optional map<string, ClassId> community_to_classid;

  /**
  * Not to advertise a prefix to a peer, if the prefix's AS-path contain that peer's AS
  */
  28: optional bool sender_suppress_as_loop;

  /**
   * Indicates global drained state
   */
  29: bgp_policy.DrainState drain_state = bgp_policy.DrainState.UNDRAINED;
  30: list<string> drained_interfaces = [];

  /**
   * Scuba category for logging certain policy evaluation result
   */
  31: optional string scuba_logging_category;

  /**
   * Switch-level limit handling prefix and path limit
   */
  32: optional BgpSwitchLimitConfig switch_limit_config;

  /**
   * Enable the VIPs limit control on vip server
   */
  35: optional bool enable_vip_server_limit;

  /**
   * Include feature knob and parameters configuration
   */
  36: optional BgpSettingConfig bgp_setting_config;

  /**
   * Config for BGP thrift server.
   */
  37: optional ThriftServerConfig thrift_server_config;

  /**
   * NetServiceFramework configuration for EBB deployments
   */
  38: optional BgpNetServiceThriftConfig net_service_config;
}
