/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

/*
 * Sample util script to illustrate how to establish end to end ping in DSF
 * topology without FSDB.
 *
 *
 * Consider a setup: RDSW A <=> FDSWs <=> RDSW B
 * During the normal mode of operation, any neighbors resolved by RDSW A
 * will be sync'ed to RDSW B wedge_agent via RDSW A FSDB and vice-versa.
 *
 * This script achieves similar effect when FSDB is not running:
 *  - Get system ports, interfaces from RDSW A.
 *  - Set those as remote system ports, remote interfaces on RDSW B.
 *  - RDSW B can then send unidirectional traffic from RDSW B to RDSW A, as
 *    neighbors directly resolved by RDSW A are now programmed and reachable
 *    from RDSW B.
 *
 * This is a sample script to illustrate the workflow. A test orchestrator for
 * a DSF test cluster without FSDB will need a more comprehensive functionality
 * on the lines below:
 *
 * Periodically run below logic for every RDSW R in the DSF cluster:
 *    - get localSysPorts, localIntfs for every other RDSW in the DSF cluster.
 *    - set isLocal = false for every neighbor in localIntf neighbor table.
 *    - merge all the localSysPorts/localIntfs received into respective lists.
 *    - set these lists as remoteSysPorts, remoteIntfs for R.
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
