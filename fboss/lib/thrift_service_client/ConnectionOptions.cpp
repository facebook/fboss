// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/thrift_service_client/ConnectionOptions.h"
#include "fboss/agent/if/gen-cpp2/ctrl_clients.h"
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_clients.h"

namespace facebook::fboss::utils {

template <>
ConnectionOptions ConnectionOptions::defaultOptions<facebook::fboss::FbossCtrl>(
    std::optional<folly::SocketAddress> dstAddr) {
  return ConnectionOptions(
      std::move(dstAddr).value_or(
          folly::SocketAddress("::1", FLAGS_wedge_agent_port)));
}

template <>
ConnectionOptions
ConnectionOptions::defaultOptions<facebook::fboss::QsfpService>(
    std::optional<folly::SocketAddress> dstAddr) {
  return ConnectionOptions(
      std::move(dstAddr).value_or(
          folly::SocketAddress("::1", FLAGS_qsfp_service_port)));
}

template <>
ConnectionOptions
ConnectionOptions::defaultOptions<facebook::fboss::fsdb::FsdbService>(
    std::optional<folly::SocketAddress> dstAddr) {
  return ConnectionOptions(
      std::move(dstAddr).value_or(folly::SocketAddress("::1", FLAGS_fsdbPort)));
}

} // namespace facebook::fboss::utils
