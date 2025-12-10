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

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbUtils.h"
#include "fboss/cli/fboss2/facebook/CmdStreamHandler.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

#include <folly/coro/AsyncPipe.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Task.h>
#include <servicerouter/client/cpp2/ServiceRouter.h>
#include <thrift/lib/cpp2/gen/module_types_h.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <unistd.h>

namespace facebook::fboss {

struct CmdStreamSubFsdbOperStateTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_FSDB_PATH;
  using ObjectArgType = utils::FsdbPath;
  using RetType = facebook::fboss::fsdb::OperState;
  using StreamRetType = folly::coro::AsyncGenerator<RetType&&, RetType>;
};

class CmdStreamSubFsdbOperState : public CmdStreamHandler<
                                      CmdStreamSubFsdbOperState,
                                      CmdStreamSubFsdbOperStateTraits> {
 public:
  using ObjectArgType = CmdStreamSubFsdbOperStateTraits::ObjectArgType;
  using RetType = CmdStreamSubFsdbOperStateTraits::RetType;
  using StreamRetType = CmdStreamSubFsdbOperStateTraits::StreamRetType;

  StreamRetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& fsdbPath) {
    auto client =
        utils::createClient<facebook::fboss::fsdb::FsdbServiceAsyncClient>(
            hostInfo);

    fsdb::OperSubRequest req;
    fsdb::OperPath path;
    path.raw() = fsdbPath.data();
    req.path() = path;
    req.protocol() = fsdb::OperProtocol::SIMPLE_JSON;
    req.subscriberId() = "fboss2-cli";
    auto gen = client->sync_subscribeOperStatePath(req);

    return std::move(gen).stream.toAsyncGenerator();
  }

  void printOutput(const RetType& result, std::ostream& out = std::cout) {
    if (!result.contents()) {
      return;
    }
    std::cout << *result.contents() << std::endl;
    if (auto meta = result.metadata(); meta) {
      dumpFsdbMetadata(*meta);
    }
  }
};

} // namespace facebook::fboss
