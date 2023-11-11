// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/Client.h"

#include <folly/SocketAddress.h>

#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"

DEFINE_bool(plaintext_fsdb_client, false, "Use plaintext FSDB clients");

namespace facebook::fboss::fsdb {
std::unique_ptr<apache::thrift::Client<FsdbService>> Client::getClient(
    const folly::SocketAddress& dstAddr,
    const std::optional<folly::SocketAddress>& srcAddr,
    std::optional<uint8_t> tos,
    const bool plaintextClient,
    folly::EventBase* eb) {
  CHECK(eb->isInEventBaseThread());
  bool usePlaintextClient = plaintextClient || dstAddr.isLoopbackAddress() ||
      FLAGS_plaintext_fsdb_client;
  return usePlaintextClient
      ? utils::createPlaintextClient<FsdbService>(dstAddr, srcAddr, eb, tos)
      : utils::tryCreateEncryptedClient<FsdbService>(dstAddr, srcAddr, eb, tos);
}
} // namespace facebook::fboss::fsdb
