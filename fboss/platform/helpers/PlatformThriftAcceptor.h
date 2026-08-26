// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <folly/IPAddress.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <wangle/acceptor/SharedSSLContextManager.h>

#include "fboss/platform/helpers/PlatformThriftAcceptorUtil.h"

namespace folly {
class EventBase;
class SocketAddress;
} // namespace folly

namespace apache::thrift {
class ThriftServer;
} // namespace apache::thrift

namespace facebook::fboss::platform::helpers {

// Connection-level authorization for platform Thrift services. At accept time
// (before any RPC dispatches) it admits only loopback peers and peers in the
// configured trusted subnets, and rejects every other plaintext connection.
// This is the FBOSS-idiomatic place to enforce auth: the platform handlers do
// not (and should not) carry per-method peer checks. Modeled on
// fboss/fsdb/server/ThriftAcceptor.h, minus the FSDB-specific TOS handling.
//
// Opt-in: installed only when --platform_enable_thrift_acceptor is set, so the
// default OSS behavior (no acceptor) is unchanged.
class PlatformThriftAcceptorFactory final
    : public wangle::AcceptorFactorySharedSSLContext {
 public:
  PlatformThriftAcceptorFactory(
      apache::thrift::ThriftServer* server,
      std::vector<folly::CIDRNetwork> trustedSubnets)
      : server_(server), trustedSubnets_(std::move(trustedSubnets)) {}
  ~PlatformThriftAcceptorFactory() override = default;

  std::shared_ptr<wangle::SharedSSLContextManager> initSharedSSLContextManager()
      override {
    // Platform OSS services run plaintext-on-loopback; no shared TLS context.
    return nullptr;
  }

  std::shared_ptr<wangle::Acceptor> newAcceptor(
      folly::EventBase* evb) override {
    class PlatformAcceptor : public apache::thrift::Cpp2Worker {
     public:
      PlatformAcceptor(
          apache::thrift::ThriftServer* server,
          std::vector<folly::CIDRNetwork> trustedSubnets)
          : apache::thrift::Cpp2Worker(server, {}),
            trustedSubnets_(std::move(trustedSubnets)) {}

      static std::shared_ptr<PlatformAcceptor> create(
          apache::thrift::ThriftServer* server,
          folly::EventBase* evb,
          std::vector<folly::CIDRNetwork> trustedSubnets) {
        auto self = std::make_shared<PlatformAcceptor>(
            server, std::move(trustedSubnets));
        self->construct(server, evb, nullptr);
        return self;
      }

      void onNewConnection(
          folly::AsyncTransportWrapper::UniquePtr socket,
          const folly::SocketAddress* address,
          const std::string& nextProtocolName,
          wangle::SecureTransportType secureTransportType,
          const wangle::TransportInfo& transportInfo) final {
        const auto clientIp = address->getIPAddress();
        if (!isTrustedClient(clientIp, trustedSubnets_)) {
          XLOG_EVERY_MS(WARNING, 5000)
              << "Rejecting Thrift connection from untrusted client "
              << clientIp.str()
              << " (not loopback or in --platform_trusted_subnets)";
          // Dropping the socket here closes the connection.
          return;
        }
        apache::thrift::Cpp2Worker::onNewConnection(
            std::move(socket),
            address,
            nextProtocolName,
            secureTransportType,
            transportInfo);
      }

     private:
      std::vector<folly::CIDRNetwork> trustedSubnets_;
    };

    return PlatformAcceptor::create(server_, evb, trustedSubnets_);
  }

 private:
  apache::thrift::ThriftServer* server_;
  std::vector<folly::CIDRNetwork> trustedSubnets_;
};

} // namespace facebook::fboss::platform::helpers
