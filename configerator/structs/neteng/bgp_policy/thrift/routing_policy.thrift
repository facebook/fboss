/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

namespace py neteng.robotron.routing_policy
namespace py3 configerator.structs.neteng.robotron
namespace cpp2 facebook.bgp.routing_policy
namespace go neteng.bgp_policy.routing_policy

include "thrift/annotation/cpp.thrift"
package "facebook.com/neteng/fboss/bgp/public_tld/configerator/structs/neteng/bgp_policy/routing_policy"

enum IPVersion {
  V4 = 1,
  V6 = 2,
}

enum MatchValueLogicOperator {
  EQUAL = 0,
  NOT_EQUAL = 1,
}

// ComparisonOperator defines the comparison method against a numeric value
// It is used in CompareNumericValue.
enum ComparisonOperator {
  // Equal: this is "eq" in EOS and "exact" in JunOS.
  EQ = 1,
  // Greater than or equal: this is "ge" in EOS and "orlonger" in JunOS
  GE = 2,
  // Less than or equal: this is "le" in EOS and "upto" in JunOS
  LE = 3,
  // Not equal
  NE = 4,
  // Greater than: matches "higher" or "longer" in JunOS
  GT = 5,
  // Less than: matches "lower" or "shorter" in JunOS
  LT = 6,
  // Range. Used to denote a range of values.
  // In FBNET the range value is used as "21..24"
  RG = 7,
}

// BooleanOperator defines logical operator on how to evaluate a list of match
// fields. AND means match all, OR means match any, NOT means invert.
enum BooleanOperator {
  AND = 1,
  OR = 2,
  NOT = 3,
}

// CompareNumericValue is a struct to compare against a numeric value
// It is used CommunityCount, BgpPolicyAtomicMatch
@cpp.MinimizePadding
struct CompareNumericValue {
  1: i32 value;
  2: ComparisonOperator compare_operator;
}

// It contains the prefix to match and the allowed length constrained
// in prefix_len_range. It is used in PrefixList.
@cpp.MinimizePadding
struct PrefixListEntry {
  // in the form of ipv4/ipv6 prefixes, including mask length
  1: string base_prefix;
  // This will be used as a sequence number that identifies the policy line
  2: optional i32 seq_num;
  // range of accepted prefix-lengths
  3: list<CompareNumericValue> prefix_len_ranges = [];
  // MatchValueLogicOperator: EQUAL is permit in EOS and NOT_EQUAL is deny
  4: MatchValueLogicOperator match_logic = MatchValueLogicOperator.EQUAL;
  // match prefixes using regex INSTEAD OF prefix_len_ranges
  5: optional string regex;

  11: optional string description;

  // IP version for this entry
  12: optional IPVersion ip_version;

  // The maximum number of subnets to accept for this prefix. Excess subnets
  // will be dropped.
  // NOTE: This field only applies to golden prefixes
  // (i.e., only when the prefix is part of TGoldenPrefixPolicy).
  13: optional i32 max_allowed_golden_prefix_subnet_count;
  // The accepted communities for this prefix.
  14: optional set<string> communities;

  100: optional string obj_uuid;
}

// PrefixList defines both prefix list inline and reference.
// It uses BooleanOperator to match AND/INVERT of the prefixes.
// Or is not applicable here. The prefixes can be a single element,
// prefix_list_names stores the names of existing PrefixList.
@cpp.MinimizePadding
struct PrefixList {
  1: string name = "";
  2: string description = "";
  3: optional i32 version;
  // inline usage, define the PrefixList here
  4: list<PrefixListEntry> prefixes = [];
  // reference usage, predefined PrefixList, using their names here
  // this is used for vendor configuration conversion, not for fboss bgp++
  5: list<string> prefix_list_names = [];
  6: BooleanOperator boolean_operator = BooleanOperator.OR;
  // added to support JunOS can have "higher" for a set of prefixes
  7: optional ComparisonOperator compare_operator;

  // Whether the list contains v4 or v6 prefixes
  // NOTE: This is not used in CRF. For CRF, the same PrefixList can contain
  // both v4 and v6 prefixes.
  11: optional IPVersion ip_version;

  100: optional string obj_uuid;
}

// Two ways to reference a PrefixList, by
// either it's name or by speficiying the object.
union PrefixListType {
  1: PrefixList prefix_list;
  2: string prefix_list_name;
}

// NeighborList defines the list of neighbors to match. It can match both name
// of the NeighborList and the inline neighbors.
@cpp.MinimizePadding
struct NeighborList {
  1: string name = "";
  2: string description = "";
  3: list<Neighbor> neighbors = [];
  4: list<string> neighbor_list_names = [];
  5: BooleanOperator boolean_operator = BooleanOperator.OR;
}

@cpp.MinimizePadding
struct TagList {
  1: string name = "";
  2: string description = "";
  3: list<string> tags = [];
  4: list<string> tag_list_names = [];
  5: BooleanOperator boolean_operator = BooleanOperator.OR;
}

struct Neighbor {
  1: string peer_address = "";
  2: string description = "";
  3: optional i32 peer_as;
}

@cpp.MinimizePadding
struct NextHop {
  1: optional i32 version;
  2: optional string next_hop_prefix;
  // example: ipv6 route 2803:6082:f822:400::/54 Null0
  3: optional string next_hop_interface;
}

// Level is mainly used in ISIS, we also use it in bgp edge policies
@cpp.MinimizePadding
struct Level {
  1: string name = "";
  2: string description = "";
  3: optional string wide_metrics_only;
  4: optional string disable;
  512: optional i32 metric;
}
