/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbSubscribers.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/core.h>
#include <folly/coro/BlockingWait.h>
#include <unistd.h>
#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

namespace facebook::fboss {

namespace {

// Most timestamp fields on OperSubscriberInfo are milliseconds-since-epoch
// (set via getCurrentTimeMs() in fboss/fsdb/oper/Subscription.h).
// "subscribedSince" is the historical exception and stores plain seconds.
// Helpers are shared with CmdShowFsdbPublishers via fsdb_cli_format in
// CmdShowFsdbUtils.h.

void printSubscriberDetail(
    const fsdb::OperSubscriberInfo& subscriber,
    std::ostream& out) {
  out << "" << std::endl;
  out << fmt::format(
             "Subscriber Id:                  {}",
             subscriber.subscriberId().value())
      << std::endl;
  out << fmt::format(
             "Type:                           {}",
             apache::thrift::util::enumNameSafe(
                 folly::copy(subscriber.type().value())))
      << std::endl;
  out << fmt::format(
             "Path:                           {}",
             utils::getSubscriptionPathStr(subscriber))
      << std::endl;
  out << fmt::format(
             "isStats:                        {}",
             folly::copy(subscriber.isStats().value()))
      << std::endl;
  out << fmt::format(
             "Subscription UID:               {}",
             fsdb_cli_format::optFieldToString<int64_t>(
                 subscriber.subscriptionUid()))
      << std::endl;

  std::string subscribedSince = "--";
  if (apache::thrift::get_pointer(subscriber.subscribedSince())) {
    subscribedSince = fsdb_cli_format::epochSecondsAsLocalTime(
        *apache::thrift::get_pointer(subscriber.subscribedSince()));
  }
  out << fmt::format("Subscribed Since:               {}", subscribedSince)
      << std::endl;

  out << fmt::format(
             "Initial Sync Completed At:      {}",
             subscriber.initialSyncCompletedAt().has_value()
                 ? fsdb_cli_format::epochMillisAsLocalTime(
                       subscriber.initialSyncCompletedAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Last Update Enqueued At:        {}",
             subscriber.lastUpdateEnqueuedAt().has_value()
                 ? fsdb_cli_format::epochMillisAsLocalTime(
                       subscriber.lastUpdateEnqueuedAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Last Update Written At:         {}",
             subscriber.lastUpdateWrittenAt().has_value()
                 ? fsdb_cli_format::epochMillisAsLocalTime(
                       subscriber.lastUpdateWrittenAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Last Enqueued Update Pub At:    {}",
             subscriber.lastEnqueuedUpdatePublishedAt().has_value()
                 ? fsdb_cli_format::epochMillisAsLocalTime(
                       subscriber.lastEnqueuedUpdatePublishedAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Last Heartbeat Sent At:         {}",
             subscriber.lastHeartbeatSentAt().has_value()
                 ? fsdb_cli_format::epochMillisAsLocalTime(
                       subscriber.lastHeartbeatSentAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Staleness (vs lastWritten):     {}",
             subscriber.lastUpdateWrittenAt().has_value()
                 ? fsdb_cli_format::stalenessFromMillis(
                       subscriber.lastUpdateWrittenAt().value())
                 : "--")
      << std::endl;
  out << fmt::format(
             "Num Updates Served:             {}",
             fsdb_cli_format::optFieldToString<int64_t>(
                 subscriber.numUpdatesServed()))
      << std::endl;
  out << fmt::format(
             "Enqueued Data Size (bytes):     {}",
             fsdb_cli_format::optFieldToString<int64_t>(
                 subscriber.enqueuedDataSize()))
      << std::endl;
  out << fmt::format(
             "Served Data Size (bytes):       {}",
             fsdb_cli_format::optFieldToString<int64_t>(
                 subscriber.servedDataSize()))
      << std::endl;
  out << fmt::format(
             "Subscription Chunks Coalesced:  {}",
             fsdb_cli_format::optFieldToString<int32_t>(
                 subscriber.subscriptionChunksCoalesced()))
      << std::endl;
  out << fmt::format(
             "Subscription Queue Watermark:   {}",
             fsdb_cli_format::optFieldToString<int32_t>(
                 subscriber.subscriptionQueueWatermark()))
      << std::endl;
}

} // namespace

CmdShowFsdbSubscribers::RetType CmdShowFsdbSubscribers::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& fsdbClientid) {
  auto client = utils::createClient<
      apache::thrift::Client<facebook::fboss::fsdb::FsdbService>>(hostInfo);

  fsdb::SubscriberIds subscribers(fsdbClientid.begin(), fsdbClientid.end());

  fsdb::SubscriberIdToOperSubscriberInfos subInfos;
  if (subscribers.empty()) {
    client->sync_getAllOperSubscriberInfos(subInfos);
  } else {
    client->sync_getOperSubscriberInfos(subInfos, subscribers);
  }
  return subInfos;
}

void CmdShowFsdbSubscribers::printOutput(
    const RetType& result,
    std::ostream& out) {
  if (CmdGlobalOptions::getInstance()->isDetailed()) {
    for (const auto& subscriberInfo : result) {
      for (const auto& subscriber : subscriberInfo.second) {
        printSubscriberDetail(subscriber, out);
      }
    }
    return;
  }

  utils::Table table;
  table.setHeader(
      {"Subscribers Id",
       "Type",
       "Raw Path",
       "isStats",
       "Subscribed Since",
       "QueueWatermark"});
  for (const auto& subscriberInfo : result) {
    for (const auto& subscriber : subscriberInfo.second) {
      std::string subscriberId =
          folly::to<std::string>(subscriber.subscriberId().value());
      auto subscriberType = apache::thrift::util::enumNameSafe(
          folly::copy(subscriber.type().value()));
      std::string subscriberPath = utils::getSubscriptionPathStr(subscriber);
      std::string subscriberIsStats =
          folly::to<std::string>(folly::copy(subscriber.isStats().value()));
      std::string queueWatermark = "--";
      if (subscriber.subscriptionQueueWatermark().has_value()) {
        queueWatermark = folly::sformat(
            "{}", subscriber.subscriptionQueueWatermark().value());
      }

      std::string subscribedSince = "--";
      if (apache::thrift::get_pointer(subscriber.subscribedSince())) {
        subscribedSince = fsdb_cli_format::epochSecondsAsLocalTime(
            *apache::thrift::get_pointer(subscriber.subscribedSince()));
      }

      table.addRow(
          {subscriberId,
           subscriberType,
           subscriberPath,
           subscriberIsStats,
           subscribedSince,
           queueWatermark});
    }
  }
  out << table << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdShowFsdbSubscribers, CmdShowFsdbSubscriberTraits>::run();

} // namespace facebook::fboss
