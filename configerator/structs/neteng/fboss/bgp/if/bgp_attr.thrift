/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

namespace cpp2 facebook.neteng.fboss.bgp_attr
namespace py neteng.fboss.bgp_attr
namespace py3 neteng.fboss
namespace go neteng.fboss.bgp_attr

include "thrift/annotation/thrift.thrift"

package "facebook.com/neteng/fboss/bgp/public_tld/configerator/structs/neteng/fboss/bgp/bgp_attr"

enum TAsPathSegType {
  AS_SET = 0x01,
  AS_SEQUENCE = 0x02,
  AS_CONFED_SEQUENCE = 0x03,
  AS_CONFED_SET = 0x04,
}

struct TAsPathSeg {
  1: TAsPathSegType seg_type;
  // deprecated: use asns_4_byte instead T113736668
  @thrift.Deprecated
  2: list<i32> asns;
  // RFC 6793, i64 = unsigned i32
  3: list<i64> asns_4_byte;
}
typedef list<TAsPathSeg> TAsPath

/**
 * AddPathCapability
 * None: do not use add path feature
 * RECEIVE: only receive additional path from peer.
 * SEND: only send additional path from peer.
 * BOTH: send/receive additional paths to/from peer.
 */
enum AddPath {
  RECEIVE = 1,
  SEND = 2,
  BOTH = 3,
}

/**
 * Control knob for advertising link bandwidth community to a peer or
 * peer-group. There are multiple options for advertising the link-bandwidth.
 *
 * The knobs classify into two, ORIGINATE and TRANSFORM.
 *   ORIGINATE - For origination the link-bandwidth community for the route is
 *               derived from configuration, either the peer to whom we're
 *               advertising or the peers associated with ECMP paths.
 *   TRANSFORM - These knobs transforms the associated/received link-bandwidth
 *               community of the ECMP paths. For the transforms to be
 *               effective, all ECMP paths must have a link-bandwidth community.
 */
enum AdvertiseLinkBandwidth {
  // (default) Do not advertise the link bandwidth
  DISABLE = 0,

  // Transform - Advertise the link-bandwidth of the best-path
  BEST_PATH = 1,

  // Originate - Set the link-bandwidth community to a specified BPS value in
  // the peer/peer-group configuration - `link_bandwidth_bps`
  SET_LINK_BPS = 2,

  // Transform - Sum the link-bandwidth associated of all ECMP paths (received
  // from peer). All ECMP paths must have a link-bandwidth else the community
  // will not be set and any existing community will be removed
  AGGREGATE_RECEIVED = 3,

  // Originate - Sum the link-bandwidth-bps of all ECMP paths peers. BPS is
  // inferred from the peer or peer-group configuration. For route programming
  // the received link-bandwidth on route is used. All ECMP path peers must have
  // `link_bandwidth_bps` configured in config, else community will be not set
  // and any existing community will be removed.
  AGGREGATE_LOCAL = 4,

  // link-bandwidth-bps set by external entity via rib-policy api. e.g. FA Controller
  RIB_POLICY_LBW = 5,
}

/**
 * Control knob for link bandwidth community association on a received route
 * from peer. Knobs here provides a way to originate the link-bandwidth even
 * if peer doesn't announce or reject the announced link-bandwidth community
 * from the peer.
 */
enum ReceiveLinkBandwidth {
  // Do not accept any link bandwidth advertised from peer. This is a safety
  // knob to ensure any un-expected link-bandwidth community received from
  // peer is rejected.
  DISABLE = 0,

  // Accept community advertised from peer if any
  ACCEPT = 1,

  // Originate - Set the link-bandwidth to the link bps value after accepting
  // the route in AdjRib-In. Any link-bandwidth community sent by peer will be
  // ignored.
  SET_LINK_BPS = 2,
}

enum TBgpAfi {
  AFI_IPV4 = 0,
  AFI_IPV6 = 1,
  AFI_ALL = 2,
}

/**
 * IP prefix, here prefix_str is the text for CIDR representation
 */
struct TIpPrefix {
  1: TBgpAfi afi;
  2: binary prefix_bin;
  3: i16 num_bits;
}

/**
 * Here i64 community = (asn << 16) + value
 */
struct TBgpCommunity {
  1: i32 asn;
  2: i32 value;
  3: i64 community;
}
