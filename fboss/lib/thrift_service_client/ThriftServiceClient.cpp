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

DEFINE_string(wedge_agent_host, "::1", "Host running wedge_agent");
DEFINE_int32(wedge_agent_port, 5909, "Port running wedge_agent");
DEFINE_string(qsfp_service_host, "::1", "Host running qsfp_service");
DEFINE_int32(qsfp_service_port, 5910, "Port running qsfp_service");

namespace facebook::fboss::utils {

std::unique_ptr<apache::thrift::Client<facebook::fboss::FbossCtrl>>
createWedgeAgentClient(
    std::optional<folly::IPAddress> ip,
    std::optional<int> port,
    folly::EventBase* eb) {
  folly::IPAddress serviceIP =
      ip ? *ip : folly::IPAddress(FLAGS_wedge_agent_host);
  int servicePort = port ? *port : FLAGS_wedge_agent_port;
  return tryCreateEncryptedClient<facebook::fboss::FbossCtrl>(
      serviceIP, servicePort, eb);
}

std::unique_ptr<apache::thrift::Client<facebook::fboss::QsfpService>>
createQsfpServiceClient(
    std::optional<folly::IPAddress> ip,
    std::optional<int> port,
    folly::EventBase* eb) {
  folly::IPAddress serviceIP =
      ip ? *ip : folly::IPAddress(FLAGS_qsfp_service_host);
  int servicePort = port ? *port : FLAGS_qsfp_service_port;
  return tryCreateEncryptedClient<facebook::fboss::QsfpService>(
      serviceIP, servicePort, eb);
}
} // namespace facebook::fboss::utils
