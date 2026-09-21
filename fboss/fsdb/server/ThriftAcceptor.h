// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <wangle/acceptor/SharedSSLContextManager.h>
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_constants.h"

namespace folly {
class EventBase;
class SocketAddress;
} // namespace folly

namespace apache {
namespace thrift {
class ThriftServer;
} // namespace thrift
} // namespace apache

namespace facebook::fboss::fsdb {

template <typename SharedSSLContextManagerClass>
class FsdbThriftAcceptorFactory final
    : public wangle::AcceptorFactorySharedSSLContext {
  // NOLINTNEXTLINE
  using ThriftAclCheckerFunc =
      std::function<bool(const folly::AsyncTransportWrapper*)>;

 public:
  FsdbThriftAcceptorFactory(
      apache::thrift::ThriftServer* server,
      std::optional<ThriftAclCheckerFunc> aclChecker,
      std::vector<folly::CIDRNetwork> trustedSubnets)
      : server_(server),
        aclChecker_(aclChecker),
        trustedSubnets_(trustedSubnets) {}
  ~FsdbThriftAcceptorFactory() override = default;

  std::shared_ptr<wangle::SharedSSLContextManager> initSharedSSLContextManager()
      override {
    if constexpr (!std::is_same<SharedSSLContextManagerClass, void>::value) {
      sharedSSLContextManager_ = std::make_shared<SharedSSLContextManagerClass>(
          server_->getServerSocketConfig());
    }
    return sharedSSLContextManager_;
  }

  std::shared_ptr<wangle::Acceptor> newAcceptor(
      folly::EventBase* evb) override {
    class FsdbAcceptor : public apache::thrift::Cpp2Worker {
      // NOLINTNEXTLINE
      using ThriftAclCheckerFunc =
          FsdbThriftAcceptorFactory::ThriftAclCheckerFunc;

     public:
      explicit FsdbAcceptor(
          apache::thrift::ThriftServer* server,
          std::optional<ThriftAclCheckerFunc> aclChecker,
          std::vector<folly::CIDRNetwork> trustedSubnets)
          : apache::thrift::Cpp2Worker(server, {}),
            aclChecker_(std::move(aclChecker)),
            trustedSubnets_(trustedSubnets) {}

      static std::shared_ptr<FsdbAcceptor> create(
          apache::thrift::ThriftServer* server,
          folly::EventBase* evb,
          std::optional<ThriftAclCheckerFunc> aclChecker,
          std::vector<folly::CIDRNetwork> trustedSubnets,
          std::shared_ptr<fizz::server::CertManager> certManager = nullptr,
          std::shared_ptr<wangle::SSLContextManager> ctxManager = nullptr,
          std::shared_ptr<const fizz::server::FizzServerContext> fizzContext =
              nullptr) {
        auto self = std::make_shared<FsdbAcceptor>(
            server, std::move(aclChecker), trustedSubnets);
        self->setFizzCertManager(certManager);
        self->setSSLContextManager(ctxManager);
        self->construct(server, evb, fizzContext);
        return self;
      }

      bool isIpInSubnets(
          const folly::IPAddress& ip,
          const std::vector<folly::CIDRNetwork>& subnets) {
        for (const auto& subnet : subnets) {
          if (ip.inSubnet(subnet.first, subnet.second)) {
            return true;
          }
        }
        return false;
      }

      void onNewConnection(
          folly::AsyncTransportWrapper::UniquePtr socket,
          const folly::SocketAddress* address,
          [[maybe_unused]] const std::string& nextProtocolName,
          wangle::SecureTransportType secureTransportType,
          const wangle::TransportInfo& transportInfo) final {
        auto clientIp = address->getIPAddress();
        auto isTrustedClient = address->isLoopbackAddress() ||
            isIpInSubnets(clientIp, trustedSubnets_);
        if (secureTransportType == wangle::SecureTransportType::NONE) {
          // plaintext connection: Allow only from trusted clients.
          if (!isTrustedClient) {
            XLOG(INFO) << "Reject plaintext connection from clientIP: "
                       << clientIp.str() << ", not in trustedSubnets";
            return;
          }
        } else {
          // TLS connection, subject to ACL enforcement.
          if (!runChecker(socket.get())) {
            return;
          }
        }
        if (!isTrustedClient) {
          // Reject connections from clients outside trusted zone that
          // are marked with traffic class NC
          int tos = 0;
          socklen_t optsize = sizeof(tos);
          auto* asyncSocket =
              socket->getUnderlyingTransport<folly::AsyncSocketTransport>();
          folly::NetworkSocket netsocket = asyncSocket->getNetworkSocket();
          auto rc = folly::netops::getsockopt(
              netsocket, IPPROTO_IPV6, IPV6_TCLASS, &tos, &optsize);
          if (rc == 0) {
            // TODO(daiweix): also block connections to fsdbPort_high_priority
            // port if not in trusted subnets
            if (tos == fsdb_common_constants::kTosForClassOfServiceNC()) {
              XLOG(INFO) << "Reject connection with TOS(" << tos
                         << ") from clientIp: " << clientIp.str()
                         << ", not in trustedSubnets";
              socket->closeNow();
              return;
            } else if (tos != 0) {
              // No TC marking at host for clients outside trusted zone.
              XLOG(INFO) << "Accept connection with TOS(" << tos
                         << ") from clientIp: " << clientIp.str()
                         << ", disable TC marking at host on return traffic.";
              tos = 0;
              folly::netops::setsockopt(
                  netsocket, IPPROTO_IPV6, IPV6_TCLASS, &tos, optsize);
            }
          }
        }
        apache::thrift::Cpp2Worker::onNewConnection(
            std::move(socket),
            address,
            std::string{"rs"},
            secureTransportType,
            transportInfo);
      }

     private:
      std::optional<ThriftAclCheckerFunc> aclChecker_;
      std::vector<folly::CIDRNetwork> trustedSubnets_;

      bool runChecker(folly::AsyncTransportWrapper* socket) noexcept {
        if (aclChecker_.has_value()) {
          try {
            if (!(*aclChecker_)(socket)) {
              // Failed ACL check and enforcement enabled.
              return false;
            }
          } catch (const std::exception& ex) {
            XLOG(FATAL) << "ERROR: Exception in aclChecker: " << ex.what();
          }
        }
        return true;
      }
    };

    if (!sharedSSLContextManager_) {
      return FsdbAcceptor::create(server_, evb, aclChecker_, trustedSubnets_);
    }
    auto acceptor = FsdbAcceptor::create(
        server_,
        evb,
        aclChecker_,
        trustedSubnets_,
        sharedSSLContextManager_->getCertManager(),
        sharedSSLContextManager_->getContextManager(),
        sharedSSLContextManager_->getFizzContext());
    sharedSSLContextManager_->addAcceptor(acceptor);
    return acceptor;
  }

 private:
  apache::thrift::ThriftServer* server_;
  std::optional<ThriftAclCheckerFunc> aclChecker_;
  std::vector<folly::CIDRNetwork> trustedSubnets_;
};

} // namespace facebook::fboss::fsdb
