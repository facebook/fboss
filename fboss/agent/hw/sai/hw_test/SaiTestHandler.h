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

#include "common/fb303/cpp/FacebookBase2.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/diag/DiagShell.h"
#include "fboss/agent/hw/sai/hw_test/gen-cpp2/SaiTestCtrl.h"

#include <thrift/lib/cpp/server/TServerEventHandler.h>

#include <folly/String.h>
#include <memory>

namespace facebook::fboss {

class DiagCmdServer;
class SaiSwitch;
class StreamingDiagShellServer;

class SaiTestHandler : virtual public SaiTestCtrlSvIf,
                       public fb303::FacebookBase2,
                       public apache::thrift::server::TServerEventHandler {
 public:
  explicit SaiTestHandler(const SaiSwitch* hw);
  ~SaiTestHandler() override;

  apache::thrift::ResponseAndServerStream<std::string, std::string>
  startDiagShell() override;
  void produceDiagShellInput(
      std::unique_ptr<std::string> input,
      std::unique_ptr<ClientInformation> client) override;

  void diagCmd(
      fbstring& result,
      std::unique_ptr<fbstring> cmd,
      std::unique_ptr<ClientInformation> client,
      int16_t serverTimeoutMsecs = 0,
      bool bypassFilter = false) override;

 private:
  // Forbidden copy constructor and assignment operator
  SaiTestHandler(SaiTestHandler const&) = delete;
  SaiTestHandler& operator=(SaiTestHandler const&) = delete;

  const SaiSwitch* hw_;
  StreamingDiagShellServer diagShell_;
  DiagCmdServer diagCmdServer_;
};

} // namespace facebook::fboss
