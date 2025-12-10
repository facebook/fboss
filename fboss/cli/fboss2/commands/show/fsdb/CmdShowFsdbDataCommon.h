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

#include "fboss/fsdb/if/FsdbModel.h"

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

#include <folly/coro/BlockingWait.h>
#include <servicerouter/client/cpp2/ServiceRouter.h>
#include <thrift/lib/cpp2/gen/module_types_h.h>
#include <unistd.h>

namespace facebook::fboss {

struct CmdShowFsdbDataCommonTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_FSDB_PATH;
  using ObjectArgType = utils::FsdbPath;
  using RetType = facebook::fboss::fsdb::OperState;
};

class CmdShowFsdbDataCommon
    : public CmdHandler<CmdShowFsdbDataCommon, CmdShowFsdbDataCommonTraits> {
 public:
  using ObjectArgType = CmdShowFsdbDataCommonTraits::ObjectArgType;
  using RetType = CmdShowFsdbDataCommonTraits::RetType;

  virtual ~CmdShowFsdbDataCommon() = default;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& fsdbPath) {
    auto client =
        utils::createClient<facebook::fboss::fsdb::FsdbServiceAsyncClient>(
            hostInfo);

    fsdb::OperGetRequest req;
    fsdb::OperPath path;
    path.raw() = fsdbPath.data();
    req.path() = path;
    req.protocol() = fsdb::OperProtocol::SIMPLE_JSON;

    RetType result;
    try {
      if (isStats()) {
        client->sync_getOperStats(result, std::move(req));
      } else {
        client->sync_getOperState(result, std::move(req));
      }
    } catch (fsdb::FsdbException& ex) {
      std::cerr << "Error from FSDB Server: " << *ex.message() << std::endl;
      throw;
    }

    return result;
  }
  void printOutput(const RetType& result, std::ostream& out = std::cout) {
    out << *result.contents() << std::endl;
    if (auto meta = result.metadata(); meta) {
      dumpFsdbMetadata(*meta);
    }
  }

 protected:
  virtual bool isStats() const = 0;
};

} // namespace facebook::fboss
