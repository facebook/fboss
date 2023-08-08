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
#ifndef IS_OSS
#if __has_feature(address_sanitizer)
#include <sanitizer/lsan_interface.h>
#endif
#endif

namespace facebook::fboss {

void SaiRepl::doRun() {
  // this blocks forever, can't hold singleton or api table must be leaky
  // singleton
  auto apiTable = SaiApiTable::getInstance().get();
  shellThread_ = std::make_unique<std::thread>(
      [switchId = saiSwitchId_, exited = &exited_, apiTable]() {
        folly::setThreadName("SaiRepl");
        SaiSwitchTraits::Attributes::SwitchShellEnable shell{true};
        exited->store(false);
        // Need to use unlocked API since this set attribute will start
        // a shell REPL loop, we can't get into that loop while holding
        // a lock. We rely on adapter implementation to make setting
        // of this attribtute thread safe.
        apiTable->switchApi().setAttributeUnlocked(switchId, shell);
        exited->store(true);
      });
}

SaiRepl::~SaiRepl() noexcept {
  /*
   * Depending on the implementation, shell thread may just be running a infinte
   * loop. Since the lifetime of REPL is same as that of our program, just
   * detach from this thread, when the program is being stopped
   */
  if (!shellThread_) {
    return;
  }
  try {
    if (!exited_.load()) {
      shellThread_->detach();
      __attribute__((unused)) auto leakedShellThread = shellThread_.release();
#ifndef IS_OSS
#if __has_feature(address_sanitizer)
      __lsan_ignore_object(leakedShellThread);
#endif
#endif
    } else {
      shellThread_->join();
      shellThread_.reset();
    }
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
