/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"

namespace facebook::fboss::utils {

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createWedgeAgentClient(
    std::optional<folly::IPAddress> ip,
    std::optional<int> port,
    folly::EventBase* eb) {
  return createPlaintextClient<facebook::fboss::FbossCtrlAsyncClient>(
      ip ? *ip : folly::IPAddress(FLAGS_wedge_agent_host),
      port ? *port : FLAGS_wedge_agent_port,
      eb);
}

std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient>
createQsfpServiceClient(
    std::optional<folly::IPAddress> ip,
    std::optional<int> port,
    folly::EventBase* eb) {
  return createPlaintextClient<facebook::fboss::QsfpServiceAsyncClient>(
      ip ? *ip : folly::IPAddress(FLAGS_qsfp_service_host),
      port ? *port : FLAGS_qsfp_service_port,
      eb);
}
} // namespace facebook::fboss::utils
