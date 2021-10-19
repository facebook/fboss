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

#include <memory>

#include "fboss/agent/MKAServicePorts.h"
#include "fboss/agent/thrift_packet_stream/BidirectionalPacketStream.h"
#include "fboss/agent/types.h"

DECLARE_double(mka_reconnect_timer);

namespace folly {
class ScopedEventBaseThread;
} // namespace folly

namespace apache::thrift {
class ScopedServerInterfaceThread;
} // namespace apache::thrift

namespace facebook::fboss {
class RxPacket;
class SwSwitch;

class MKAServiceManager : public BidirectionalPacketAcceptor {
 public:
  static constexpr uint16_t ETHERTYPE_EAPOL = 0x888E;
  explicit MKAServiceManager(SwSwitch* sw);
  virtual ~MKAServiceManager() override;

  void handlePacket(std::unique_ptr<RxPacket> packet);
  virtual void recvPacket(TPacket&& packet) override;
  uint16_t getServerPort() const;
  bool isConnectedToMkaServer() const {
    return stream_ ? stream_->isConnectedToServer() : false;
  }

 private:
  std::string getPortName(PortID portId) const;
  std::shared_ptr<BidirectionalPacketStream> stream_;
  SwSwitch* swSwitch_;
  std::unique_ptr<apache::thrift::ScopedServerInterfaceThread> serverThread_;
  std::unique_ptr<folly::ScopedEventBaseThread> clientThread_;
  std::unique_ptr<folly::ScopedEventBaseThread> timerThread_;
};

} // namespace facebook::fboss
