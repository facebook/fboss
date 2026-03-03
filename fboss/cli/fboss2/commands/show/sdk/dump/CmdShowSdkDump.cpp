/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/sdk/dump/CmdShowSdkDump.h"

#include <boost/algorithm/string.hpp>
#include <folly/FileUtil.h>
#include <folly/testing/TestUtil.h>
#include <fstream>
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

CmdShowQsfpSdkDump::RetType CmdShowQsfpSdkDump::queryClient(
    const HostInfo& hostInfo) {
  auto client =
      utils::createClient<facebook::fboss::QsfpServiceAsyncClient>(hostInfo);

  if (!tempSdkFile.get()) {
    // Creating exception safe unique file which will get removed on exiting
    // scope for unique pointer
    tempSdkFile = std::make_unique<folly::test::TemporaryFile>(
        folly::StringPiece() /* namePrefix */,
        folly::fs::path() /* dir */,
        folly::test::TemporaryFile::Scope::UNLINK_ON_DESTRUCTION /* scope */);
  }

  bool rc = client->sync_getSdkState(tempSdkFile->path().string());
  return rc;
}

void CmdShowQsfpSdkDump::printOutput(const RetType& rc, std::ostream& out) {
  std::ifstream sdkDumpStream;
  std::string outputLine;

  if (!rc) {
    out << "Getting SDK state failed" << std::endl;
    return;
  }
  out << "Printing SDK state:" << std::endl;

  // Read the temp file with SDK dump
  if (!folly::readFile(tempSdkFile->path().string().c_str(), outputLine)) {
    out << "Reading temp file " << tempSdkFile->path().string() << " failed"
        << std::endl;
    return;
  }
  // Print temp file
  out << outputLine << std::endl;
}

CmdShowAgentSdkDump::RetType CmdShowAgentSdkDump::queryClient(
    const HostInfo& hostInfo) {
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  std::string debugDump{};
  client->sync_getHwDebugDump(debugDump);
  return debugDump;
}

void CmdShowAgentSdkDump::printOutput(const RetType& rc, std::ostream& out) {
  std::ifstream sdkDumpStream;
  std::string outputLine;

  out << "Printing Agent SDK state:" << std::endl;
  out << rc;
}

} // namespace facebook::fboss
