/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <iostream>

#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient> createAgentClient(
    const std::string ip) {
  static auto constexpr kConnTimeout = 1000;
  static auto constexpr kSendTimeout = 5000;
  static auto constexpr kRecvTimeout = 45000;
  static auto constexpr port = 5909;

  auto eb = folly::EventBaseManager::get()->getEventBase();
  auto addr = folly::SocketAddress(folly::IPAddress(ip), port);
  auto sock = folly::AsyncSocket::newSocket(eb, addr, kConnTimeout);
  sock->setSendTimeout(kSendTimeout);
  auto channel =
      apache::thrift::HeaderClientChannel::newChannel(std::move(sock));
  channel->setTimeout(kRecvTimeout);

  return std::make_unique<facebook::fboss::FbossCtrlAsyncClient>(
      std::move(channel));
}

int main() {
  auto getClient = createAgentClient("1001::1"); // RDSW A IP
  auto setClient = createAgentClient("1001::2"); // RDSW B IP

  std::map<std::string, std::string> pathToState;

  auto renamePath = [&pathToState](std::string oldPath, std::string newPath) {
    auto node = pathToState.extract(oldPath);
    node.key() = newPath;
    pathToState.insert(std::move(node));
  };

  try {
    // get from RDSW A
    getClient->sync_getCurrentStateJSONForPaths(
        pathToState, {"systemPortMaps", "interfaceMaps"});

    // process: local => remote
    // TODO, set isLocal = false
    renamePath("systemPortMaps", "remoteSystemPortMaps");
    renamePath("interfaceMaps", "remoteInterfaceMaps");

    for (auto& [path, state] : pathToState) {
      std::cout << "######## Path: " << path << " state: " << state;
    }

    // set on RDSW B
    setClient->sync_patchCurrentStateJSONForPaths(pathToState);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    exit(1);
  }

  return 0;
}
