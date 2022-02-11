/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <boost/core/noncopyable.hpp>

#include <folly/SpinLock.h>
#include <folly/Synchronized.h>
#include <map>

namespace facebook::fboss {

class RxPacket;

class PacketObserverIf : public boost::noncopyable {
 public:
  virtual ~PacketObserverIf() {}
  void handlePacketRx(const RxPacket* pkt) noexcept {
    packetReceived(pkt);
  }

 private:
  virtual void packetReceived(const RxPacket* pkt) noexcept = 0;
};

class PacketObservers : public boost::noncopyable {
 public:
  // EventObserver() noexcept {}
  void registerPacketObserver(
      PacketObserverIf* observer,
      const std::string& name);
  void unregisterPacketObserver(
      PacketObserverIf* observer,
      const std::string& name);

  void packetReceived(const RxPacket* pkt);

 private:
  folly::Synchronized<std::map<std::string, PacketObserverIf*>> pktObservers_;
};

} // namespace facebook::fboss
