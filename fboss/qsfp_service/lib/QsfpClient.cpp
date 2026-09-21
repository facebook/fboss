/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/qsfp_service/lib/QsfpClient.h"

DEFINE_int32(
    qsfp_service_recv_timeout,
    5000,
    "Receive timeout(ms) from qsfp service");

namespace facebook {
namespace fboss {

// static
apache::thrift::RpcOptions QsfpClient::getRpcOptions() {
  apache::thrift::RpcOptions opts;
  opts.setTimeout(std::chrono::milliseconds(FLAGS_qsfp_service_recv_timeout));
  return opts;
}

} // namespace fboss
} // namespace facebook
