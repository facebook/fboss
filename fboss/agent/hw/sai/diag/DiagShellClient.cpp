/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <servicerouter/client/cpp2/ServiceRouter.h>
#include "fboss/agent/hw/sai/switch/gen-cpp2/SaiCtrlAsyncClient.h"

#include <folly/Exception.h>
#include <folly/FileUtil.h>
#include <folly/SocketAddress.h>
#include <folly/init/Init.h>

#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseManager.h>

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

#include <thread>

namespace {

std::unique_ptr<facebook::fboss::SaiCtrlAsyncClient> getStreamingClient(
    folly::EventBase* evb,
    const folly::IPAddress& ip) {
  folly::SocketAddress addr{ip, 5909};
  return std::make_unique<facebook::fboss::SaiCtrlAsyncClient>(
      apache::thrift::RocketClientChannel::newChannel(
          apache::thrift::async::TAsyncSocket::UniquePtr(
              new apache::thrift::async::TAsyncSocket(evb, addr))));
}

void subscribeToDiagShell(folly::EventBase* evb, const folly::IPAddress& ip) {
  auto client = getStreamingClient(evb, ip);
  auto responseAndStream = client->sync_startDiagShell();
  folly::writeFull(
      STDOUT_FILENO,
      responseAndStream.response.c_str(),
      responseAndStream.response.size());
  auto streamFuture =
      std::move(responseAndStream.stream)
          .subscribeExTry(
              evb,
              [evb](auto&& t) {
                if (t.hasValue()) {
                  const auto& shellOut = t.value();
                  folly::writeFull(
                      STDOUT_FILENO, shellOut.c_str(), shellOut.size());
                  // Typically, neither the error, nor completed events will
                  // occur, we expect most streams to end when the client is
                  // terminated by the user.
                } else if (t.hasException()) {
                  auto msg = folly::exceptionStr(std::move(t.exception()));
                  // N.B., can't use folly logging, because it messes with the
                  // terminal mode such that backspace, ctrl+D, etc.. don't seem
                  // to work anymore.
                  std::cout << "error in stream: " << msg << "\n";
                  evb->terminateLoopSoon();
                } else {
                  std::cout << "stream completed!\n";
                  evb->terminateLoopSoon();
                }
              })
          .futureJoin();
  evb->loop();
}

void handleStdin(folly::EventBase* evb, const folly::IPAddress& ip) {
  auto client = getStreamingClient(evb, ip);
  ssize_t nread;
  constexpr ssize_t bufSize = 512;
  std::array<char, bufSize> buf;
  while (true) {
    if ((nread = ::read(STDIN_FILENO, buf.data(), bufSize)) < 0) {
      folly::throwSystemError("failed to read from stdin");
    } else if (nread == 0) {
      break;
    } else {
      std::string input(buf.data(), nread);
      try {
        // TODO: fill in ClientInformation, or get rid of it in the API
        facebook::fboss::ClientInformation ci;
        client->sync_produceDiagShellInput(input, ci);
      } catch (const std::exception& e) {
        std::cout << "cli caught server exception " << e.what() << "\n";
      }
    }
  }
}

} // namespace

/*
 * Connect to the FBOSS agent SAI streaming thrift service,
 *
 * start a diag shell session to get a thrift stream; handle new stream data by
 * printing it.
 *
 * start a thread reading standard input and sending it to the FBOSS agent
 */

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  folly::EventBase streamEvb;
  folly::EventBase stdinEvb;

  std::thread streamT([&streamEvb]() {
    subscribeToDiagShell(&streamEvb, folly::IPAddress{"::1"});
  });

  std::thread readStdinT(
      [&stdinEvb]() { handleStdin(&stdinEvb, folly::IPAddress{"::1"}); });

  readStdinT.join();
  streamEvb.terminateLoopSoon();
  streamT.join();
}
