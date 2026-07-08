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
