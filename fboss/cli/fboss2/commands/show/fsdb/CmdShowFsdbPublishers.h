/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <fboss/fsdb/if/gen-cpp2/fsdb_common_types.h>
#include <folly/coro/BlockingWait.h>
#include <thrift/lib/cpp2/gen/module_types_h.h>

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"
#include "servicerouter/client/cpp2/ServiceRouter.h"

#include <unistd.h>

namespace facebook::fboss {

struct CmdShowFsdbPublisherTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_FSDB_CLIENT_ID;
  using ObjectArgType = utils::FsdbClientId;
  using RetType = facebook::fboss::fsdb::PublisherIdToOperPublisherInfo;
};

class CmdShowFsdbPublishers
    : public CmdHandler<CmdShowFsdbPublishers, CmdShowFsdbPublisherTraits> {
 public:
  using ObjectArgType = CmdShowFsdbPublisherTraits::ObjectArgType;
  using RetType = CmdShowFsdbPublisherTraits::RetType;
  RetType queryClient(
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
  void printOutput(const RetType& result, std::ostream& out = std::cout) {
    utils::Table table;
    table.setHeader({"Publishers Id", "Type", "Raw Path", "isStats"});
    for (auto publisherInfo : result) {
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
};

} // namespace facebook::fboss
