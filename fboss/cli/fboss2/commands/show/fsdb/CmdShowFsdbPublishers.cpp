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

#include <folly/coro/BlockingWait.h>
#include <unistd.h>
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

namespace facebook::fboss {

CmdShowFsdbPublishers::RetType CmdShowFsdbPublishers::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& fsdbClientid) {
  auto client =
      utils::createClient<facebook::fboss::fsdb::FsdbServiceAsyncClient>(
          hostInfo);

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
  utils::Table table;
  table.setHeader({"Publishers Id", "Type", "Raw Path", "isStats"});
  for (const auto& publisherInfo : result) {
    for (auto publisher : publisherInfo.second) {
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

} // namespace facebook::fboss
