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

#include <boost/algorithm/string.hpp>
#include <folly/testing/TestUtil.h>
#include <fstream>
#include <unordered_set>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

struct CmdShowQsfpSdkDumpTraits : public ReadCommandTraits {
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using RetType = bool;
};

class CmdShowQsfpSdkDump
    : public CmdHandler<CmdShowQsfpSdkDump, CmdShowQsfpSdkDumpTraits> {
 public:
  using RetType = CmdShowQsfpSdkDumpTraits::RetType;
  std::unique_ptr<folly::test::TemporaryFile> tempSdkFile;

  RetType queryClient(const HostInfo& hostInfo) {
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

  void printOutput(const RetType& rc, std::ostream& out = std::cout) {
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
};

struct CmdShowAgentSdkDumpTraits : public ReadCommandTraits {
  using ParentCmd = void;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using RetType = std::string;
};

class CmdShowAgentSdkDump
    : public CmdHandler<CmdShowAgentSdkDump, CmdShowAgentSdkDumpTraits> {
 public:
  using RetType = CmdShowAgentSdkDumpTraits::RetType;
  std::unique_ptr<folly::test::TemporaryFile> tempSdkFile;

  RetType queryClient(const HostInfo& hostInfo) {
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    std::string debugDump{};
    client->sync_getHwDebugDump(debugDump);
    return debugDump;
  }

  void printOutput(const RetType& rc, std::ostream& out = std::cout) {
    std::ifstream sdkDumpStream;
    std::string outputLine;

    out << "Printing Agent SDK state:" << std::endl;
    out << rc;
  }
};
} // namespace facebook::fboss
