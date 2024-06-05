/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/QsfpServiceSignalHandler.h"

#include "fboss/qsfp_service/QsfpServiceHandler.h"

#include <folly/executors/FunctionScheduler.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <chrono>
#include <csignal>

namespace facebook::fboss {
QsfpServiceSignalHandler::QsfpServiceSignalHandler(
    folly::EventBase* eventBase,
    folly::FunctionScheduler* functionScheduler,
    std::shared_ptr<apache::thrift::ThriftServer> qsfpServer,
    std::shared_ptr<QsfpServiceHandler> qsfpServiceHandler)
    : AsyncSignalHandler(eventBase),
      functionScheduler_(functionScheduler),
      qsfpServer_(qsfpServer),
      qsfpServiceHandler_(qsfpServiceHandler) {
  registerSignalHandler(SIGINT);
  registerSignalHandler(SIGTERM);
}

using namespace std::chrono;
void QsfpServiceSignalHandler::signalReceived(int signum) noexcept {
  XLOG(INFO) << "[Exit] Signal received: " << signum;
  steady_clock::time_point begin = steady_clock::now();

  // stop accepting new connections to qsfp_service thrift server
  qsfpServer_->stopListening();
  steady_clock::time_point thriftServerStopped = steady_clock::now();
  XLOG(INFO)
      << "[Exit] Stopped thrift server listening. Stop time: "
      << duration_cast<duration<float>>(thriftServerStopped - begin).count();

  // Set graceful eiting flag to true so that refreshStateMachine can break
  qsfpServiceHandler_->getTransceiverManager()->setGracefulExitingFlag();

  // stop stats collection and state machine refresh
  functionScheduler_->shutdown();
  steady_clock::time_point functionSchedulerStopped = steady_clock::now();
  XLOG(INFO)
      << "[Exit] Stopped stats collection and state machine FunctionScheduler. "
      << "Stop time: "
      << duration_cast<duration<float>>(
             functionSchedulerStopped - thriftServerStopped)
             .count();

  qsfpServiceHandler_->getTransceiverManager()->gracefulExit();
  steady_clock::time_point gracefulExitDone = steady_clock::now();
  XLOG(INFO)
      << "[Exit] Total qsfp_service Exit time: "
      << duration_cast<duration<float>>(gracefulExitDone - begin).count();

  exit(0);
}
} // namespace facebook::fboss
