/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/diag/SaiRepl.h"

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"

#include <folly/system/ThreadName.h>

namespace facebook::fboss {

void SaiRepl::run() {
  shellThread_ = std::make_unique<std::thread>([switchId = switchId_]() {
    folly::setThreadName("Sai Repl");
    SaiSwitchTraits::Attributes::SwitchShellEnable shell{true};
    auto rv =
        SaiApiTable::getInstance()->switchApi().setAttribute(switchId, shell);
    saiCheckError(rv, "Unable to start shell thread");
  });
  running_ = true;
}

SaiRepl::~SaiRepl() noexcept {
  /*
   * Depending on the implementation, shell thread may just be running a infinte
   * loop. Since the lifetime of REPL is same as that of our program, just
   * detach from this thread, when the program is being stopped
   */
  try {
    shellThread_->detach();
  } catch (...) {
    XLOG(FATAL) << "Failed to detach shell thread during repl tear down";
  }
}

bool SaiRepl::running() const {
  return running_;
}

std::string SaiRepl::getPrompt() const {
  return ">>> ";
}

std::vector<folly::File> SaiRepl::getStreams() const {
  std::vector<folly::File> ret;
  ret.reserve(2);
  ret.emplace_back(folly::File(STDIN_FILENO));
  ret.emplace_back(folly::File(STDOUT_FILENO));
  return ret;
}

} // namespace facebook::fboss
