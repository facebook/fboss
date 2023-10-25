// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/fsdb/client/Client.h"

#include <folly/SocketAddress.h>

#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <servicerouter/client/cpp2/ServiceRouter.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"

DEFINE_bool(plaintext_fsdb_client, false, "Use plaintext FSDB clients");

namespace facebook::fboss::fsdb {
namespace {
// TODO: allow option for SR client for offbox clients
std::unique_ptr<apache::thrift::Client<FsdbService>> createSRClient(
    const std::string& ip,
    const int port) {
  XLOG(INFO) << "get async client for " << ip << ":" << port;

  facebook::servicerouter::ServiceOptions serviceOptions;
  serviceOptions["single_host"] = {ip, std::to_string(port)};
  serviceOptions["svc_select_count"] = {"1"};

  facebook::servicerouter::ConnConfigs connConfigs;
  connConfigs["thrift_transport"] = "rocket";

  return std::make_unique<apache::thrift::Client<FsdbService>>(
      facebook::servicerouter::cpp2::getClientFactory().getChannel(
          "", serviceOptions, connConfigs));
}
} // namespace

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
