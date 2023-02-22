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

template <typename ClientT>
std::unique_ptr<apache::thrift::Client<ClientT>> tryCreateEncryptedClient(
    const folly::SocketAddress& dstAddr,
    const std::optional<folly::SocketAddress>& srcAddr,
    folly::EventBase* eb,
    std::optional<uint8_t> tos) {
  // default to plaintext in oss
  return createPlaintextClient<ClientT>(dstAddr, srcAddr, eb);
}

template std::unique_ptr<apache::thrift::Client<facebook::fboss::FbossCtrl>>
tryCreateEncryptedClient(
    const folly::SocketAddress& dstAddr,
    const std::optional<folly::SocketAddress>& srcAddr,
    folly::EventBase* eb,
    std::optional<uint8_t> tos);
template std::unique_ptr<apache::thrift::Client<facebook::fboss::QsfpService>>
tryCreateEncryptedClient(
    const folly::SocketAddress& dstAddr,
    const std::optional<folly::SocketAddress>& srcAddr,
    folly::EventBase* eb,
    std::optional<uint8_t> tos);
} // namespace facebook::fboss::utils
