/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FbossEventBase.h"
#include "fboss/agent/hw/sai/switch/gen-cpp2/SaiCtrlAsyncClient.h"
#include "folly/Memory.h"

#include <folly/Exception.h>
#include <folly/FileUtil.h>
#include <folly/Range.h>
#include <folly/ScopeGuard.h>
#include <folly/SocketAddress.h>
#include <folly/init/Init.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseManager.h>
#include "fboss/agent/hw/sai/diag/DiagShellClient.h"

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <csignal>
#include <iostream>
#include <thread>

DEFINE_string(host, "::1", "The host to connect to");
DEFINE_int32(port, 5909, "The port to connect to");

namespace {
using folly::AsyncSignalHandler;
using folly::ByteRange;
using folly::IPAddress;
using folly::IPAddressV6;

const IPAddress getIPFromHost(const std::string& hostname) {
  if (IPAddress::validate(hostname)) {
    return IPAddress(hostname);
  }
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  struct addrinfo* result = nullptr;
  auto rv = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
  SCOPE_EXIT {
    LOG(INFO) << "Finished getting IP for " << hostname;
    freeaddrinfo(result);
  };
  if (rv < 0) {
    LOG(ERROR) << "Could not get an IP for " << hostname;
    return IPAddress();
  }
  struct addrinfo* res;
  for (res = result; res != nullptr; res = res->ai_next) {
    if (res->ai_addr->sa_family == AF_INET) { // IPV4
      struct sockaddr_in* sp = (struct sockaddr_in*)res->ai_addr;
      return IPAddress::fromLong(sp->sin_addr.s_addr);
    } else if (res->ai_addr->sa_family == AF_INET6) { // IPV6
      struct sockaddr_in6* sp = (struct sockaddr_in6*)res->ai_addr;
      return IPAddress::fromBinary(ByteRange(
          static_cast<unsigned char*>(&(sp->sin6_addr.s6_addr[0])),
          IPAddressV6::byteCount()));
    }
  }
  LOG(ERROR) << "Could not get an IP for " << hostname;
  return IPAddress();
}

void subscribeToDiagShell(folly::EventBase* evb, const IPAddress& ip) {
  auto client = utility::getStreamingClient(evb, ip, FLAGS_port);
  auto responseAndStream = client->sync_startDiagShell();
  folly::writeFull(
      STDOUT_FILENO,
      responseAndStream.response.c_str(),
      responseAndStream.response.size());
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
              LOG(ERROR) << "error in stream: " << msg;
              evb->terminateLoopSoon();
            } else {
              LOG(INFO) << "stream completed!";
              evb->terminateLoopSoon();
            }
          })
      .detach();
  evb->loop();
}

void handleStdin(
    folly::EventBase* evb,
    const IPAddress& ip,
    const bool* shouldStop) {
  auto client = utility::getStreamingClient(evb, ip, FLAGS_port);
  ssize_t nread;
  constexpr ssize_t bufSize = 512;
  std::array<char, bufSize> buf;
  while (true) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(STDIN_FILENO, &readSet);
    // Checks every second if we should stop the process.
    struct timeval timeout = {1, 0};
    if (select(STDIN_FILENO + 1, &readSet, nullptr, nullptr, &timeout) > 0) {
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
          LOG(ERROR) << "cli caught server exception " << e.what();
        }
        // When user enters quit, finish the stdin thread
        if (input.find("quit") == 0 || *shouldStop) {
          break;
        }
      }
    }
    if (*shouldStop) {
      break;
    }
  }
  evb->terminateLoopSoon();
}

class SignalHandler : public AsyncSignalHandler {
 public:
  SignalHandler(folly::EventBase* eventBase, bool* shouldStop)
      : AsyncSignalHandler(eventBase), shouldStop_(shouldStop) {
    registerSignalHandler(SIGINT);
    registerSignalHandler(SIGTERM);
  }
  void signalReceived(int /*signum*/) noexcept override {
    getEventBase()->terminateLoopSoon();
    *shouldStop_ = true;
  }

 private:
  bool* shouldStop_;
};

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
  folly::init(&argc, &argv);
  facebook::fboss::FbossEventBase streamEvb{"DiagShellClientStreamEventBase"};
  facebook::fboss::FbossEventBase stdinEvb{"DiagShellClientStdinEventBase"};

  bool stopThread = false;
  // Converts the host to IP address if a hostname is given
  IPAddress hostIP = getIPFromHost(FLAGS_host);
  // No host given
  if (hostIP.empty()) {
    LOG(ERROR) << "No host given to connect";
    exit(1);
  }

  LOG(INFO) << "Connecting to: " << hostIP;
  std::thread streamT(
      [&streamEvb, &hostIP]() { subscribeToDiagShell(&streamEvb, hostIP); });

  std::thread readStdinT([&stdinEvb, &hostIP, &stopThread]() {
    handleStdin(&stdinEvb, hostIP, &stopThread);
  });

  SignalHandler signalHandler(&streamEvb, &stopThread);

  readStdinT.join();
  streamEvb.terminateLoopSoon();
  streamT.join();
}
