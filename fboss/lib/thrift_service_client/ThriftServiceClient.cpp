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
DEFINE_int32(bgp_service_port, 6909, "Port running bgpd");

namespace facebook::fboss::utils {

std::unique_ptr<apache::thrift::Client<facebook::fboss::FbossCtrl>>
createWedgeAgentClient(
    const std::optional<folly::SocketAddress>& dstAddr,
    folly::EventBase* eb) {
  return tryCreateEncryptedClient<facebook::fboss::FbossCtrl>(
      ConnectionOptions::defaultOptions<facebook::fboss::FbossCtrl>(dstAddr),
      eb);
}

std::unique_ptr<apache::thrift::Client<facebook::fboss::QsfpService>>
createQsfpServiceClient(
    const std::optional<folly::SocketAddress>& dstAddr,
    folly::EventBase* eb) {
  return tryCreateEncryptedClient<facebook::fboss::QsfpService>(
      ConnectionOptions::defaultOptions<facebook::fboss::QsfpService>(dstAddr),
      eb);
}

std::unique_ptr<apache::thrift::Client<facebook::fboss::fsdb::FsdbService>>
createFsdbClient(ConnectionOptions options, folly::EventBase* eb) {
  return options.getPreferEncrypted()
      ? tryCreateEncryptedClient<facebook::fboss::fsdb::FsdbService>(
            options, eb)
      : createPlaintextClient<facebook::fboss::fsdb::FsdbService>(options, eb);
}

} // namespace facebook::fboss::utils
