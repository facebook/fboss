// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/IPAddress.h>
#include <folly/SocketAddress.h>
#include <folly/io/SocketOptionMap.h>
#include <folly/io/async/AsyncSocketTransport.h>
#include <folly/io/async/EventBase.h>

DECLARE_string(wedge_agent_host);
DECLARE_int32(wedge_agent_port);
DECLARE_string(qsfp_service_host);
DECLARE_int32(qsfp_service_port);
DECLARE_int32(bgp_service_port);

namespace facebook::fboss::utils {

class ConnectionOptions {
 public:
  explicit ConnectionOptions(folly::SocketAddress dstAddr)
      : dstAddr_(std::move(dstAddr)) {}
  ConnectionOptions(std::string dstIp, uint16_t dstPort)
      : ConnectionOptions(folly::SocketAddress(std::move(dstIp), dstPort)) {}
  ConnectionOptions(folly::IPAddress dstIp, uint16_t dstPort)
      : ConnectionOptions(folly::SocketAddress(std::move(dstIp), dstPort)) {}

  enum class TrafficClass : uint8_t { DEFAULT, NC };

  folly::SocketAddress getDstAddr() const {
    return dstAddr_;
  }

  ConnectionOptions& setConnectionTimeout(std::chrono::milliseconds timeout) {
    connTimeoutMs_ = timeout;
    return *this;
  }

  uint32_t getConnTimeoutMs() const {
    return connTimeoutMs_.count();
  }

  ConnectionOptions& setSendTimeout(std::chrono::milliseconds timeout) {
    sendTimeoutMs_ = timeout;
    return *this;
  }

  uint32_t getSendTimeoutMs() const {
    return sendTimeoutMs_.count();
  }

  ConnectionOptions& setRecvTimeout(std::chrono::milliseconds timeout) {
    recvTimeoutMs_ = timeout;
    return *this;
  }

  uint32_t getRecvTimeoutMs() const {
    return recvTimeoutMs_.count();
  }

  ConnectionOptions& setSrcAddr(std::string srcIp, uint16_t srcPort = 0) {
    srcAddr_ = folly::SocketAddress(std::move(srcIp), srcPort);
    return *this;
  }

  ConnectionOptions& setSrcAddr(std::optional<folly::SocketAddress> srcAddr) {
    srcAddr_ = std::move(srcAddr);
    return *this;
  }

  folly::SocketAddress getSrcAddr() const {
    return srcAddr_ ? *srcAddr_ : folly::AsyncSocketTransport::anyAddress();
  }

  ConnectionOptions& setTrafficClass(TrafficClass tc) {
    tc_ = tc;
    return *this;
  }

  ConnectionOptions& setHostname(std::string hostname) {
    hostname_ = std::move(hostname);
    return *this;
  }

  std::string getHostname() const {
    return hostname_.value_or(dstAddr_.getAddressStr());
  }

  ConnectionOptions& setPreferEncrypted(bool preferEncrypted) {
    preferEncrypted_ = preferEncrypted;
    return *this;
  }

  bool getPreferEncrypted() const {
    return preferEncrypted_;
  }

  folly::SocketOptionMap getSocketOptionMap() const {
    folly::SocketOptionMap sockOptsMap;
    if (auto tos = getTos()) {
      sockOptsMap.insert(
          {folly::SocketOptionKey{IPPROTO_IPV6, IPV6_TCLASS}, *tos});
    }
    return sockOptsMap;
  }

  std::optional<uint8_t> getTos() const {
    if (tc_ == TrafficClass::NC) {
      return kTosForClassOfServiceNC;
    }
    return std::nullopt;
  }

  template <typename Service>
  static ConnectionOptions defaultOptions(
      std::optional<folly::SocketAddress> dstAddr = std::nullopt);

 private:
  // DSCP for Network Control is 48
  // 8-bit TOS = 6-bit DSCP followed by 2-bit ECN
  static constexpr uint8_t kTosForClassOfServiceNC = 48 << 2;
  static constexpr uint32_t kConnTimeout = 1000;
  static constexpr uint32_t kSendTimeout = 5000;
  static constexpr uint32_t kRecvTimeout = 45000;

  folly::SocketAddress dstAddr_;
  std::chrono::milliseconds connTimeoutMs_{kConnTimeout};
  std::chrono::milliseconds sendTimeoutMs_{kSendTimeout};
  std::chrono::milliseconds recvTimeoutMs_{kRecvTimeout};
  std::optional<folly::SocketAddress> srcAddr_;
  std::optional<TrafficClass> tc_;
  std::optional<std::string> hostname_;
  bool preferEncrypted_ = true;
};

} // namespace facebook::fboss::utils
