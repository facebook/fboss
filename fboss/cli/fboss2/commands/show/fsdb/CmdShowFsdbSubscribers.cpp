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

#include <folly/coro/BlockingWait.h>
#include <unistd.h>
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

namespace facebook::fboss {

CmdShowFsdbSubscribers::RetType CmdShowFsdbSubscribers::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& fsdbClientid) {
  auto client =
      utils::createClient<facebook::fboss::fsdb::FsdbServiceAsyncClient>(
          hostInfo);

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
  utils::Table table;
  table.setHeader(
      {"Subscribers Id",
       "Type",
       "Raw Path",
       "isStats",
       "Subscribed Since",
       "QueueWatermark"});
  for (const auto& subscriberInfo : result) {
    for (auto subscriber : subscriberInfo.second) {
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
        time_t timestamp = static_cast<time_t>(
            *apache::thrift::get_pointer(subscriber.subscribedSince()));
        std::tm tm{};
        localtime_r(&timestamp, &tm);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %T");
        subscribedSince = oss.str();
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

} // namespace facebook::fboss
