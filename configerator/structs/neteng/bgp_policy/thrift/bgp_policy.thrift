/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

include "thrift/annotation/cpp.thrift"
include "thrift/annotation/thrift.thrift"

include "configerator/structs/neteng/bgp_policy/thrift/nsf_policy.thrift"
include "configerator/structs/neteng/bgp_policy/thrift/routing_policy.thrift"

package "facebook.com/neteng/fboss/bgp/public_tld/configerator/structs/neteng/bgp_policy/bgp_policy"

namespace py neteng.robotron.bgp_policy
namespace py3 configerator.structs.neteng.robotron
namespace cpp2 facebook.bgp.bgp_policy
namespace go neteng.bgp_policy.bgp_policy

// Each statement can apply a policy terminating action
// If no statement specifies this, then the default action
// applied on the policy is taken.
enum FlowControlAction {
  ACCEPT = 1,
  DENY = 2,
  NEXT_TERM = 3,
  NEXT_POLICY = 4,
  LOG_AND_NEXT_TERM = 5,
  LOG_AND_ACCEPT = 6,
  LOG_AND_DENY = 7,
}

enum DIRECTION {
  IN = 1,
  OUT = 2,
}

enum IsisLevel {
  LEVEL_1 = 1,
  LEVEL_2 = 2,
}

enum AfiSafi {
  IPV4_UNICAST = 1,
  IPV6_UNICAST = 2,
}

enum RouteType {
  EXTERNAL = 1,
  INTERNAL = 2,
}

enum ProtocolType {
  AGGREGATE = 1,
  BGP = 2,
  DIRECT = 3,
  LOCAL = 4,
  // "STATIC" will cause conflicts in thrift reflection generated symbols with the cpp static keyword
  @cpp.Name{value = "STATIC_PROTOCOL"}
  STATIC = 5,
  ISIS = 6,
}

// BGP community type
enum CommunityType {
  NORMAL = 1,
  EXTENDED = 2,
  LARGE = 3,
}

// Indicates which attribute of the BGP message
// we're acting on.
enum BgpAttrChangeActionType {
  AS_PATH_PREPEND = 1,
  SET_LOCAL_PREF = 3,
  ORIGIN = 4,
  NEXT_HOP = 5,
  AS_PATH = 6,
  MED = 7,
  // COMMUNITY_LIST actions
  COMMUNITY_LIST_ADD = 11,
  COMMUNITY_LIST_SET = 12,
  COMMUNITY_LIST_REMOVE = 13,
  // EXT_COMMUNITY_LIST actions
  EXT_COMMUNITY_LIST_ADD = 14,
  EXT_COMMUNITY_LIST_SET = 15,
  EXT_COMMUNITY_LIST_REMOVE = 16,
}

enum Origin {
  IGP = 1,
  EGP = 2,
  INCOMPLETE = 3,
}

// Type of actions a BGP policy can take:
// 1- Actions that modify route properties.
// 2- Actions that modify the flow of policy evaluation.
// A third type of action (not defined yet) can be included
// to indicate NOOP operations like logging
union BgpPolicyActionTypes {
  1: BgpAttrChangeActionType route_action;
  2: FlowControlAction flow_action;
}

enum BgpPolicyActionTypesValues {
  ROUTE_ACTION = 1,
  FLOW_ACTION = 2,
}

struct Community {
  1: CommunityType type;
  2: string name;
  3: optional string description;
  4: string value;
  // Unique identifier for this Community
  100: optional string obj_uuid;
}

// Two ways to reference a Community, by
// either its name or by specifying the object.
union CommunityRefType {
  1: Community community;
  2: string community_name;
}

// CommunityList defines a list of communities.
// It uses BooleanOperator to match AND/OR/INVERT of the list of communities.
// The inline community definition is in communities.
// The reference of previously defined CommunityList is in community_lists.
@cpp.MinimizePadding
struct CommunityList {
  1: string name = "";
  2: string description = "";
  // inline usage, define the list of communities here
  // a valid community can be regex and string represenation of community
  @thrift.Deprecated
  3: optional list<string> communities;
  // reference usage, predefined CommunityList, using their names here
  @thrift.Deprecated
  4: optional list<string> community_list_names;

  5: routing_policy.BooleanOperator boolean_operator = routing_policy.BooleanOperator.OR;
  6: optional bool exact_match;

  // List of Community members of this CommunityList
  11: optional list<CommunityRefType> members;
  // Unique identifier for this as community list.
  100: optional string obj_uuid;
}

// Two ways to reference a CommunityList, by
// either its name or by speficiying the object.
union CommunityListType {
  1: CommunityList community_list;
  2: string community_list_name;
}

// LocalPreference defines local preference.
// It uses BooleanOperator to match AND/OR/INVERT of the list of
// local_preference_list_names. The inline defintiion is
// in local_pref. The reference of previously defined LocalPreference is
// in local_preference_list_names.
@cpp.MinimizePadding
struct LocalPreference {
  1: string name = "";
  2: string description = "";
  // uint32_t value
  3: optional i64 local_pref;

  // Attributes 4,5,6 are deprecated
  @thrift.Deprecated
  4: optional i64 add_value;
  @thrift.Deprecated
  5: optional list<string> local_preference_list_names;
  @thrift.Deprecated
  6: routing_policy.BooleanOperator boolean_operator = routing_policy.BooleanOperator.OR;

  100: optional string obj_uuid;
}

@cpp.MinimizePadding
struct AsPath {
  1: string name;
  2: optional string description;
  // As Path string is handled as a regexp
  3: string asn_regexp;
  // Unique identifier for this as path.
  100: optional string obj_uuid;
}

// Two ways to reference an AsPath, by
// either its name or by specifying the object.
union AsPathType {
  1: AsPath as_path;
  2: string as_path_name;
}

// Similar to a PrefixListEntry. Represents an AsPath entry in
// an AsPathList. Allows reversing the match result.
@cpp.MinimizePadding
struct AsPathListEntry {
  1: string name = "";
  2: string description = "";

  3: AsPathType as_path;

  // If we need to reverse the match result, use
  // MatchValueLogicOperator.NOT_EQUAL
  4: routing_policy.MatchValueLogicOperator match_logic_type = routing_policy.MatchValueLogicOperator.EQUAL;
  5: optional i64 sequence_number;

  100: optional string obj_uuid;
}

// AsPathList defines a list of aspaths.
@cpp.MinimizePadding
struct AsPathList {
  1: string name = "";
  2: string description = "";

  // attributes 3, 4 and 5 are deprecated
  // Use as_path_list instead
  @thrift.Deprecated
  3: optional list<string> as_paths;
  @thrift.Deprecated
  4: optional list<string> as_path_list_names;
  @thrift.Deprecated
  5: routing_policy.BooleanOperator boolean_operator = routing_policy.BooleanOperator.OR;

  // For matching purposes this list behaves like an OR. As long as one
  // entry matches, the match is considered to be True. For each entry
  // the match can also be inverted at the AsPathListEntry level.
  10: list<AsPathListEntry> as_path_list = [];

  100: optional string obj_uuid;
}

struct CommunityCount {
  1: string name = "";
  2: routing_policy.CompareNumericValue compare_numeric_value;
}

// BgpPolicyAtomicMatchType defines the types of match,
// for both on path and prefix.
enum BgpPolicyAtomicMatchType {
  AS_PATH_LEN = 1, // AS path length without considering confeds
  AS_PATH = 2, // This maps to as_path attribute
  COMMUNITY_LIST = 3,
  ORIGIN = 4,
  PREFIX_LIST = 5,
  NEXT_HOP = 6,
  NEIGHBOR_LIST = 7,
  ROUTE_TYPE = 8,
  COMMUNITY_COUNT = 9,
  INTERFACE = 10,
  LOCAL_PREFERENCE = 11,
  METRIC = 12,
  TAG_LIST = 13, // MPLS taggings, used in OpenConfig
  MED = 14,
  FAMILY = 15, //Address family
  LEVEL = 16,
  ROUTE_FILTER = 17, // similar to prefix_list, juniper config
  PROTOCOL = 18, // bgp, isis, etc.
  AS_PATH_LEN_WITH_CONFED = 19, // AS path length including confeds
  ALWAYS = 20, // no criteria, always match
  WEIGHT = 21, // BGP weight
}

// BgpPolicyAtomicMatch is the policy match basic unit.
// BgpPolicyAtomicMatchType tells which of the following fields to use.
@cpp.MinimizePadding
struct BgpPolicyAtomicMatch {
  1: BgpPolicyAtomicMatchType type;
  2: optional list<routing_policy.CompareNumericValue> as_path_len_filter;
  @thrift.Deprecated
  3: optional AsPathList as_path_filters;
  @thrift.Deprecated
  4: optional CommunityList communities_filter;
  5: optional Origin origin;
  @thrift.Deprecated
  7: optional routing_policy.PrefixList prefix_filters;
  @thrift.Deprecated
  8: optional string route_type;
  9: optional routing_policy.NeighborList neighbor_set_filter;
  10: optional routing_policy.NextHop nexthop_filter;
  11: optional i64 med; // uint32 value
  @thrift.Deprecated
  13: optional routing_policy.Level level;
  14: optional string match_iface;
  @thrift.Deprecated
  15: optional string family;
  16: optional CommunityCount community_count;
  17: optional LocalPreference local_preference;
  18: optional routing_policy.TagList tag_set;
  @thrift.Deprecated
  19: optional string protocol;
  20: optional list<
    routing_policy.CompareNumericValue
  > as_path_len_with_confed_filter;
  // Sequence number for this match item. This can be used to enforce ordering
  // or in cases where the configuration defines an explicit sequence number
  // this is how it's defined.
  31: optional i64 sequence_number;
  // Route type
  32: optional RouteType routetype;
  // Protocol type
  33: optional ProtocolType protocol_type;
  // Address family
  34: optional AfiSafi afi_safi;
  // ISIS level match
  35: optional IsisLevel isis_level;
  // AsPathList match
  36: optional AsPathList as_path_list;
  // Community List match
  37: optional CommunityListType community_list;
  // PrefixList match
  38: optional routing_policy.PrefixListType prefix_list;
  // If we need to reverse the match result, use
  // MatchValueLogicOperator.NOT_EQUAL
  40: routing_policy.MatchValueLogicOperator match_logic_type = routing_policy.MatchValueLogicOperator.EQUAL;
  41: optional routing_policy.CompareNumericValue weight; // uint16 value

  100: optional string obj_uuid;
}

// BgpPolicyMatch composes multiple BgpPolicyAtomicMatch together using
// BooleanOperator. Match_entries incluses the list of inline
// BgpPolicyAtomicMatch. Policy_match_entry_names contains the names of already
// defined BgpPolicyAtomicMatch.
@cpp.MinimizePadding
struct BgpPolicyMatch {
  1: string name = "";
  2: string description = "";
  3: routing_policy.BooleanOperator match_logic_type = routing_policy.BooleanOperator.AND;
  4: list<BgpPolicyAtomicMatch> match_entries = [];

  // Unique identifier for this match object.
  100: optional string obj_uuid;
}

// BgpPolicyActionType defines the types of action.
enum BgpPolicyActionType {
  // Used in BgpPolicyAction
  AS_PATH_PREPEND = 1,
  COMMUNITY_LIST = 2,
  SET_LOCAL_PREF = 3,
  ORIGIN = 4,
  PERMIT = 5,
  DENY = 6,
  CONTINUE = 7,
  NEXT_HOP = 8,
  AS_PATH = 9,
  MED = 10,
  GOTO = 11,
  LBW_EXT_COMMUNITY = 12,
  // See AsPathToAsSetAction
  AS_PATH_TO_AS_SET = 13,
  EXT_COMMUNITY_LIST = 14,
  WEIGHT = 15,
}

// SetNextHop is the action to set nexthop value.
// Nexthop can be set to eithr nexthop ip or set to itself.
struct SetNextHop {
  1: bool set_self = 0;
  2: optional routing_policy.NextHop next_hop;
}

// It defines how to apply the communities.
// deprecated: CommunityAction is no longer used
enum CommunityActionType {
  ADD = 1,
  SET = 2,
  REMOVE = 3,
}

// CommunityAction contains the list of communities to apply,
// and how to apply it. Communities is used for inline definition,
// community_action_names is used for reference by name
// deprecated: Use COMMUNITY_LIST_ADD... in policy action instead
struct CommunityAction {
  1: string name = "";
  2: string description = "";
  @thrift.Deprecated
  3: optional list<string> communities;
  @thrift.Deprecated
  4: optional list<string> community_action_list_names;
  5: CommunityActionType action_type;

  // Define list of normal Community this action applies to.
  // the community list can be specified by either its name or the actual definition
  11: optional list<CommunityListType> community_lists;
}

// Link Bandwidth Extended Community Action Type
enum LbwExtCommunityActionType {
  // common actions for both recv/advertise
  DISABLE = 1, // pruneLbw()
  SET_LINK_BPS = 2, // set lbw with link-bps defined in config

  /*
  * actions ONLY for recv
  */
  ACCEPT = 3, // does nothing, take Lbw as is
  DECODE_ALL = 10, // decode all values in non-standard lbw and pass the values to RIB

  /*
   * actions ONLY for advertise
   */
  BEST_PATH = 4, // announce the best path's lbw
  AGGREGATE_LOCAL = 5, // set lbw with aggregate-local (from config)
  AGGREGATE_RECEIVED = 6, // set lbw with aggregate-received (from peers)
  // the following actions must work in conjunction with encoding_scheme/id
  ENCODE_AGGREGATE_RECEIVED_OVERWRITE = 7, // encode aggregate-received standard lbw and overwrite lbw
  ENCODE_MULTIPATH = 8, // encode size of multipath and modify non-standard lbw
  ENCODE_SWITCH_ID = 9, // encode index of the switch (e.g., 1,2,3...) and modify non-standard lbw
  DECODE_AGGREGATE_CAPACITY_OVERWRITE = 11, // decode and aggregate capacity in encoded lbw and overwrite as standard lbw
}

// Link Bandwidth Extended Community Action
struct LbwExtCommunityAction {
  1: LbwExtCommunityActionType type;
  2: optional nsf_policy.NsfTeWeightEncoding encoding_scheme; // this tells us the layout of the bits of lbw
  3: optional i32 encoding_id; // which part of the encoded lbw to store/extract the value
}

// From RFC 4360:
// Each Extended Community is encoded as an 8-octet quantity, as
// follows:
// - Type Field  : 1 or 2 octets
// - Value Field : Remaining octets
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |  Type high    |  Type low(*)  |                               |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+          Value                |
// |                                                               |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// If the ExtCommunity is of regular class, value size is 7 octets.
// If the ExtCommunity is of extended class, value size is 6 octets.
//
// The structs in this thrift file define our representation of
// an extended community.

@cpp.MinimizePadding
struct ExtCommunity {
  // -------- Core ExtCommunity fields ----------
  // Indicates the type of the ext community. The value will be decoded
  // as uint8, but we use i16 so there is no ambiguity
  // with the most significant bit (MSB) between languages during cast.
  1: optional i16 type_high;
  // If this ExtCommunity is of extended type, type_low should be populated.
  // Using i16 type has same reasoning as above.
  2: optional i16 type_low;

  // @value holds binary data; string and binary are treated the same in cpp.
  // We use string here to be consistent with bgp_policy::Community.
  3: optional string value;

  // -------- Human readable fields -------------
  4: optional string name;
  5: optional string description;

  // Unique identifier for this ext community
  100: optional string obj_uuid;
}

// Container for a list of communities as part of an update action.
// The possible actions at bgp_policy::BgpAttrChangeActionType
// are
//   EXT_COMMUNITY_LIST_ADD
//   EXT_COMMUNITY_LIST_SET
//   EXT_COMMUNITY_LIST_REMOVE
struct ExtCommunityAction {
  1: optional list<ExtCommunity> ext_communities;

  // Unique identifier for ext community list instance.
  100: optional string obj_uuid;
}

enum MedActionType {
  // Set med to med_value.
  SET = 1,
  // Update existing med value by +/- value.
  UPDATE = 2,
  // Set the MED value to the IGP cost of the NextHop.
  IGP = 3,
}

@cpp.MinimizePadding
struct MedAction {
  // uint32 value
  1: i64 med_value;
  2: MedActionType med_action_type;
  //^[+-][0-9]+$
  3: optional string update_pattern;

  100: optional string obj_uuid;
}

enum WeightActionType {
  // Set weight to weight_value.
  SET = 1,
} // Can be extended later if neeeded

@cpp.MinimizePadding
struct WeightAction {
  // uint16 value
  1: i32 weight_value;
  2: WeightActionType weight_action_type;

  100: optional string obj_uuid;
}

// Aspath prepending action defintiion.
@cpp.MinimizePadding
struct SetAsPathPrepend {
  // uint32 value
  1: i64 asn;
  2: i32 repeat_times;

  100: optional string obj_uuid;
}

// Compress the entire AS_PATH into a single AS_SET.
// - All existing AS numbers will be present in the resulting AS_SET.
// - The resulting AS_PATH will have length 1.
// - Note that policy actions are applied *before* the local AS number is
//   prepended to the AS_PATH. So the ultimate output AS_PATH will look like
//   [AS_SEQ(local_asn), AS_SET(...)].
//
// This struct corresponds to enum `BgpPolicyActionType.AS_PATH_TO_AS_SET`.
struct AsPathToAsSetAction {}

// BgpPolicyActionType determines which field of the following is used.
@cpp.MinimizePadding
struct BgpPolicyAction {
  @thrift.Deprecated
  1: BgpPolicyActionType type;
  2: optional SetAsPathPrepend set_as_path_prepend;
  @thrift.Deprecated
  3: optional CommunityAction community_action;
  4: optional LocalPreference set_local_pref;
  5: optional Origin set_origin;
  6: optional SetNextHop set_nexthop;
  7: optional MedAction med_action;
  8: optional AsPathToAsSetAction as_path_to_as_set_action;

  // Action types
  10: optional BgpPolicyActionTypes action_type;

  // used with FlowControlAction.NEXT_POLICY
  // points to a valid BgpPolicyStatement.id
  11: optional string next_policy_id;
  //
  // used with FlowControlAction.NEXT_TERM.
  // points to a valid BgpPolicyTerm.name
  12: optional string next_term_id;

  // List of Communities to act on
  13: optional CommunityListType community_list;

  // List of asn to overwrite the as path field
  // new as path will have one as segment with
  // this whole list in the asSequence;
  // Any 0 asns passed in by config will be replaced by
  // peer's asn;
  // if this list is empty, as path vector will have
  // no segments.
  14: optional list<i64> as_path_overwrite_list;

  // link bandwidth extended community action
  15: optional LbwExtCommunityAction lbw_ext_community_action;

  // List of ExtCommunities that @action_type applies to.
  // If @action_type is not set, then we apply @type.
  16: optional ExtCommunityAction ext_communities_action;

  17: optional WeightAction weight_action;

  // Optional ordering attribute
  90: optional i64 sequence_number;

  100: optional string obj_uuid;
}

// BgpPolicyTerm contrains list of match (criteria) and actions.
@cpp.MinimizePadding
struct BgpPolicyTerm {
  1: string name = "";
  2: string description = "";
  @thrift.Deprecated
  3: optional BgpPolicyMatch policy_match_entries;
  4: list<BgpPolicyAction> policy_action_entries = [];

  // term_miss_action is applied when a prefix does not match
  // policy_match_entries.
  // - Defaults to NEXT_TERM.
  // - Using term_miss_action = DENY allows early exit in policy statement, which
  // improves policy efficiency.
  // - Using term_miss_action = LOG_AND_NEXT_TERM allows to see what prefixes will be
  // denied on adding a new policy term.
  5: FlowControlAction term_miss_action = FlowControlAction.NEXT_TERM;

  9: optional list<BgpPolicyMatch> policy_matches;
  // Boolean operation to apply to the list of policy_matches
  10: routing_policy.BooleanOperator match_logic_type = routing_policy.BooleanOperator.AND;

  // Sequence number for this term. This is how term ordering is
  // enforced within a given Policy
  11: optional i64 sequence_number;
  // Unique identifier for this policy.
  100: optional string obj_uuid;
}

// BgpPolicyStatement is the base policy struct.
// Name can be used in other policies. It contrains a list of policy entries.
// Policy entries are evaluated sequentially.
@cpp.MinimizePadding
struct BgpPolicyStatement {
  1: string name;
  2: optional string description;
  3: string policy_version;
  4: list<BgpPolicyTerm> policy_entries = [];

  // Default action for this policy. This is used if no other
  // term specifies a terminating condition. When used in this
  // context, the enum would be either ACCEPT or DENY
  11: FlowControlAction result = FlowControlAction.DENY;
  // Unique identifier for this policy.
  100: optional string obj_uuid;
}

// configuration root, contains multiple policies
@cpp.MinimizePadding
struct BgpPolicies {
  1: list<BgpPolicyStatement> bgp_policy_statements;
  2: list<CommunityList> community_lists = [];
  3: list<AsPathList> aspath_lists = [];
  4: list<routing_policy.PrefixList> prefix_lists = [];
  5: list<AsPath> as_paths = [];
  6: list<Community> communities = [];

  100: optional string obj_uuid;
}

enum DrainState {
  UNDRAINED = 0,
  DRAINED = 1,
  WARM_DRAINED = 2,
  SOFT_DRAINED = 3,
}
