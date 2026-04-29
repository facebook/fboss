/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbPublishers.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/core.h>
#include <folly/coro/BlockingWait.h>
#include <unistd.h>
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

namespace facebook::fboss {

namespace {

// Timestamp semantics for the fields below come from
// fboss/fsdb/server/ServiceHandler.cpp:
// - connectedAt is epoch seconds (set via std::time(nullptr) at L469).
// - initialSyncCompletedAt, lastUpdateReceivedAt, lastHeartbeatReceivedAt,
//   lastUpdatePublishedAt are epoch ms (set via getCurrentTimeMs()).
// Helpers are shared with CmdShowFsdbSubscribers via fsdb_cli_format in
// CmdShowFsdbUtils.h.

void printPublisherDetail(
    const fsdb::OperPublisherInfo& publisher,
    std::ostream& out) {
  out << "" << std::endl;
  out << fmt::format(
             "Publisher Id:                   {}",
             publisher.publisherId().value())
      << std::endl;
  out << fmt::format(
             "Type:                           {}",
             apache::thrift::util::enumNameSafe(
                 folly::copy(publisher.type().value())))
      << std::endl;
  out << fmt::format(
             "Path:                           {}",
             folly::join("/", publisher.path().value().raw().value()))
      << std::endl;
  out << fmt::format(
             "isStats:                        {}",
             folly::copy(publisher.isStats().value()))
      << std::endl;
  out << fmt::format(
             "isExpectedPath:                 {}",
             folly::copy(publisher.isExpectedPath().value()))
      << std::endl;
  out << fmt::format(
             "Connected At:                   {}",
             publisher.connectedAt().has_value()
                 ? fsdb_cli_format::epochSecondsAsLocalTime(
                       publisher.connectedAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Initial Sync Completed At:      {}",
             publisher.initialSyncCompletedAt().has_value()
                 ? fsdb_cli_format::epochMillisAsLocalTime(
                       publisher.initialSyncCompletedAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Last Update Received At:        {}",
             publisher.lastUpdateReceivedAt().has_value()
                 ? fsdb_cli_format::epochMillisAsLocalTime(
                       publisher.lastUpdateReceivedAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Last Heartbeat Received At:     {}",
             publisher.lastHeartbeatReceivedAt().has_value()
                 ? fsdb_cli_format::epochMillisAsLocalTime(
                       publisher.lastHeartbeatReceivedAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Last Update Published At:       {}",
             publisher.lastUpdatePublishedAt().has_value()
                 ? fsdb_cli_format::epochMillisAsLocalTime(
                       publisher.lastUpdatePublishedAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Staleness (vs lastReceived):    {}",
             publisher.lastUpdateReceivedAt().has_value()
                 ? fsdb_cli_format::stalenessFromMillis(
                       publisher.lastUpdateReceivedAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Num Updates Received:           {}",
             fsdb_cli_format::optFieldToString<int64_t>(
                 publisher.numUpdatesReceived()))
      << std::endl;
  out << fmt::format(
             "Received Data Size (bytes):     {}",
             fsdb_cli_format::optFieldToString<int64_t>(
                 publisher.receivedDataSize()))
      << std::endl;
}

} // namespace

CmdShowFsdbPublishers::RetType CmdShowFsdbPublishers::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& fsdbClientid) {
  auto client = utils::createClient<
      apache::thrift::Client<facebook::fboss::fsdb::FsdbService>>(hostInfo);

  fsdb::PublisherIds publishers(fsdbClientid.begin(), fsdbClientid.end());

  fsdb::PublisherIdToOperPublisherInfo pubInfos;
  if (publishers.empty()) {
    client->sync_getAllOperPublisherInfos(pubInfos);
  } else {
    client->sync_getOperPublisherInfos(pubInfos, publishers);
  }
  return pubInfos;
}

void CmdShowFsdbPublishers::printOutput(
    const RetType& result,
    std::ostream& out) {
  if (CmdGlobalOptions::getInstance()->isDetailed()) {
    for (const auto& publisherInfo : result) {
      for (const auto& publisher : publisherInfo.second) {
        printPublisherDetail(publisher, out);
      }
    }
    return;
  }

  utils::Table table;
  table.setHeader({"Publishers Id", "Type", "Raw Path", "isStats"});
  for (const auto& publisherInfo : result) {
    for (const auto& publisher : publisherInfo.second) {
      std::string publisherId =
          folly::to<std::string>(publisher.publisherId().value());
      auto publisherType = apache::thrift::util::enumNameSafe(
          folly::copy(publisher.type().value()));
      std::string publisherPath =
          folly::join("/", publisher.path().value().raw().value());
      std::string publisherIsStats =
          folly::to<std::string>(folly::copy(publisher.isStats().value()));

      table.addRow(
          {publisherId, publisherType, publisherPath, publisherIsStats});
    }
  }
  out << table << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdShowFsdbPublishers, CmdShowFsdbPublisherTraits>::run();

} // namespace facebook::fboss
