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

#include "fboss/agent/hw/sai/api/Types.h"

#include <folly/File.h>
#include <folly/Format.h>
#include <folly/logging/xlog.h>

#include <thread>
#include <vector>

namespace facebook::fboss {
class SaiSwitch;

class SaiRepl {
 public:
  explicit SaiRepl(SwitchSaiId switchId) : switchId_(switchId) {}
  ~SaiRepl() noexcept;
  void run();
  bool running() const;
  std::string getPrompt() const;
  // Get the list of streams the repl expects the shell to dup2 with
  // the pty slave fd passed into the repl constructor
  std::vector<folly::File> getStreams() const;

 private:
  std::unique_ptr<std::thread> shellThread_;
  const SwitchSaiId switchId_;
  bool running_{false};
};

} // namespace facebook::fboss
