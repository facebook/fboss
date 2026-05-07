/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

namespace cpp2 facebook.neteng.fboss.bgp.thrift
namespace py3 neteng.fboss

include "common/fb303/if/fb303.thrift"
include "neteng/fboss/bgp/if/BgpStructs.thrift"
include "common/network/if/Address.thrift"
include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

exception TBgpServiceException {
  @thrift.ExceptionMessage
  1: string message;
}

struct TBgpRouteDelta {
  1: BgpStructs.Update2EorWrapper update2OrEor;
  2: optional i64 onDeviceTimeStampMs; // When the message was advertised by the device.
}

/**
 * Extends TBgpService and implements stream APIs as streams are only
 * supported in C++
 */
service TBgpServiceStream extends fb303.FacebookService {
  // Subscribe to route updates from this device; Will send out multi path
  // routes stored inside the RIB
  //
  // subscriberName: Client should provide task name, hostname, or some unique
  // id.  This is only used for documentation purposes
  stream<TBgpRouteDelta> subscribe(1: string subscriberName) throws (
    1: TBgpServiceException ex,
  );
}
