/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/diag/PythonRepl.h"

namespace facebook::fboss {

PythonRepl::PythonRepl(int fd) : fd_(fd) {}

PythonRepl::~PythonRepl() noexcept {
  /*
   * Due to a deficiency in Py_Main (and its alternative, PyRun_InteractiveLoop)
   * we cannot cleanly terminate the infinite loops in the pyThread nor the
   * producerThread. For that reason, DiagShell has the same lifetime as
   * SaiHandler, so it is only destroyed when FBOSS is exiting and it is safe
   * to detach the threads instead of joining them.
   */
  try {
    pyThread_->detach();
  } catch (...) {
    XLOG(WARNING) << "Failed to detach python thread during repl tear down";
  }
}

void PythonRepl::doRun() {
  pyThread_ = std::make_unique<std::thread>(
      [this]() mutable { runPythonInterpreter(); });
}

std::string PythonRepl::getPrompt() const {
  return ">>> ";
}

std::vector<folly::File> PythonRepl::getStreams() const {
  std::vector<folly::File> ret;
  ret.reserve(2);
  ret.emplace_back(folly::File(STDIN_FILENO));
  ret.emplace_back(folly::File(STDOUT_FILENO));
  return ret;
}

} // namespace facebook::fboss
