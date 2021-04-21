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

#include "fboss/cli/fboss2/CLI11/CLI.hpp"

#include <memory>

namespace facebook::fboss {

class CmdGlobalOptions {
 public:
  CmdGlobalOptions() = default;
  ~CmdGlobalOptions() = default;
  CmdGlobalOptions(const CmdGlobalOptions& other) = delete;
  CmdGlobalOptions& operator=(const CmdGlobalOptions& other) = delete;

  // Static function for getting the CmdGlobalOptions folly::Singleton
  static std::shared_ptr<CmdGlobalOptions> getInstance();

  void init(CLI::App& app);

  std::string getHost() {
    return host_;
  }

  std::string getLogLevel() {
    return logLevel_;
  }

 private:
  void initAdditional(CLI::App& app);

  std::string host_{"localhost"};
  std::string logLevel_{"DBG0"};
};

} // namespace facebook::fboss
