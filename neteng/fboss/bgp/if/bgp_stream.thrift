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
