/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/PacketObserver.h"
#include <folly/logging/xlog.h>
#include "fboss/agent/FbossError.h"

class RxPacket;

namespace facebook::fboss {

void PacketObservers::packetReceived(const RxPacket* pkt) {
  // notify about the pkt received to the interested observers
  auto pktObserversMap = pktObservers_.rlock();
  for (auto& pktObserver : *pktObserversMap) {
    try {
      auto observer = pktObserver.second;
      observer->handlePacketRx(pkt);
    } catch (const std::exception& ex) {
      XLOG(FATAL) << "Error: " << folly::exceptionStr(ex)
                  << "in packet event notification for : "
                  << pktObserver.second;
    }
  }
}

void PacketObservers::registerPacketObserver(
    PacketObserverIf* observer,
    const std::string& name) {
  auto pktObservers = pktObservers_.wlock();
  if (pktObservers->find(name) != pktObservers->end()) {
    throw FbossError("Observer was already added: ", name);
  }
  XLOG(INFO) << "Register: " << name << " as packet observer";
  pktObservers->emplace(name, observer);
}

void PacketObservers::unregisterPacketObserver(
    PacketObserverIf* /*unused */,
    const std::string& name) {
  auto pktObservers = pktObservers_.wlock();
  if (!pktObservers->erase(name)) {
    throw FbossError("Observer erase failed for:", name);
  }
  XLOG(INFO) << "Unergister: " << name << " as packet observer";
}

} // namespace facebook::fboss
