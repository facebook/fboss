/*
 * Copyright 2004- Facebook. All rights reserved
 */

namespace cpp2 facebook.nettools.bgplib
namespace go nettools.bgplib.if.BmpStructs
namespace php BmpStructs
namespace py nettools.bgplib.BmpStructs
namespace py3 nettools.bgplib
namespace py.asyncio nettools.bgplib_asyncio.BmpStructs

include "neteng/fboss/bgp/if/BgpStructs.thrift"
include "common/network/if/Address.thrift"

package "facebook.com/nettools/bgplib"

/**
 * Structure containing a BgpUpdate and additional information from BMP.
 *
 * bgpPeerId: The peer's ID from the BGP session.
 * bgpPeerAsn: The peer's ASN from the BGP session's configuration.
 *   The peer's ASN may be  different than the first ASN in the AS path, for
 *   instance for route servers.
 * bmpPeerIp: The IP address of the BMP peer that the update was received from.
 */
struct BmpBgpUpdate {
  1: BgpStructs.BgpUpdate update;
  2: i64 bgpPeerId;
  3: i64 bgpPeerAsn;
  4: Address.BinaryAddress bmpPeerIp;
}
