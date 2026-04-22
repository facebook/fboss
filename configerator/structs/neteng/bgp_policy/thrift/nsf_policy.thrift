/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

namespace cpp2 facebook.bgp.nsf_policy
namespace py3 neteng.bgp_policy.thrift
namespace py neteng.bgp_policy.thrift.nsf_policy
namespace go neteng.bgp_policy.nsf_policy

include "thrift/annotation/scope.thrift"
include "thrift/annotation/thrift.thrift"

package "facebook.com/neteng/fboss/bgp/public_tld/configerator/structs/neteng/bgp_policy/nsf_policy"

@thrift.RuntimeAnnotation
@scope.Field
struct NsfTeWeightEncodingFieldAnnotation {
  // if this is true, the encoded value will be factored into the calculation of standard ucmp lbw
  // when converting from encoded lbw to standard lbw
  1: bool is_ucmp_capacity = false;
}

// When this is specified in BGP policy, BGP++ will be able to encode/decode the different
// values using exclusive sets of bits of an extended community (uint32 value).
// The names of the fields are actually not important for BGP++, as it only needs
// the id and the value to know order and size of each value.
struct NsfL2TeWeightEncoding {
  // identifier
  1: i32 rack_id;
  2: i32 plane_id;
  // capacity
  @NsfTeWeightEncodingFieldAnnotation{is_ucmp_capacity = true}
  3: i32 remote_rack_capacity;
  4: i32 spine_capacity;
  5: i32 local_rack_capacity;
}

union NsfTeWeightEncoding {
  1: NsfL2TeWeightEncoding l2_encoding;
}
