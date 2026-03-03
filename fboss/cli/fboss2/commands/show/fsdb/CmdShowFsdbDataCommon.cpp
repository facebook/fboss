/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/fsdb/CmdShowFsdbDataCommon.h"

#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

#include <folly/coro/BlockingWait.h>
#include <unistd.h>

namespace facebook::fboss {

CmdShowFsdbDataCommon::RetType CmdShowFsdbDataCommon::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& fsdbPath) {
  auto client = utils::createClient<
      apache::thrift::Client<facebook::fboss::fsdb::FsdbService>>(hostInfo);

  fsdb::OperGetRequest req;
  fsdb::OperPath path;
  path.raw() = fsdbPath.data();
  req.path() = std::move(path);
  req.protocol() = fsdb::OperProtocol::SIMPLE_JSON;

  RetType result;
  try {
    if (isStats()) {
      client->sync_getOperStats(result, req);
    } else {
      client->sync_getOperState(result, req);
    }
  } catch (fsdb::FsdbException& ex) {
    std::cerr << "Error from FSDB Server: " << *ex.message() << std::endl;
    throw;
  }

  return result;
}

void CmdShowFsdbDataCommon::printOutput(
    const RetType& result,
    std::ostream& out) {
  out << apache::thrift::can_throw(*result.contents()) << std::endl;
  if (auto meta = result.metadata(); meta) {
    dumpFsdbMetadata(*meta);
  }
}

} // namespace facebook::fboss
