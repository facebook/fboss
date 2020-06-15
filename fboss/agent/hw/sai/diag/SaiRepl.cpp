/*
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

void SaiRepl::doRun() {
  shellThread_ = std::make_unique<std::thread>([switchId = switchId_]() {
    folly::setThreadName("SaiRepl");
    SaiSwitchTraits::Attributes::SwitchShellEnable shell{true};
    // Need to use unlocked API since this set attribute will start
    // a shell REPL loop, we can't get into that loop while holding
    // a lock. We rely on adapter implementation to make setting
    // of this attribtute thread safe.
    SaiApiTable::getInstance()->switchApi().setAttributeUnlocked(
        switchId, shell);
  });
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
