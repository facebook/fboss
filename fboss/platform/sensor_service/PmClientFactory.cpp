// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/PmClientFactory.h"

#include <folly/io/async/AsyncSSLSocket.h>
#include <folly/io/async/SSLOptions.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

#include "common/fbwhoami/FbWhoAmI.h"
#include "common/services/cpp/FacebookIdentityVerifier.h"
#include "common/services/cpp/TLSConstants.h"
#include "configerator/structs/infrasec/if/gen-cpp2/acl_constants.h"
#include "configerator/structs/infrasec/if/gen-cpp2/acl_types.h"
#include "infrasec/authorization/BaseIdentityUtil.h"
#include "security/ca/lib/certpathpicker/CertPathPicker.h"

namespace facebook::fboss::platform::sensor_service {
std::unique_ptr<
    apache::thrift::Client<platform_manager::PlatformManagerService>>
PmClientFactory::create() const {
  auto certKeyPair =
      facebook::security::CertPathPicker::getClientCredentialPaths(true);
  if (certKeyPair.first.empty() || certKeyPair.second.empty()) {
    return createPlainTextClient();
  } else {
    return createSecureClient(certKeyPair);
  }
}

std::unique_ptr<
    apache::thrift::Client<platform_manager::PlatformManagerService>>
PmClientFactory::createPlainTextClient() const {
  auto socket = folly::AsyncSocket::UniquePtr(
      new folly::AsyncSocket(evb_.get(), pmSocketAddr_));
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<
      apache::thrift::Client<platform_manager::PlatformManagerService>>(
      std::move(channel));
}

std::unique_ptr<
    apache::thrift::Client<platform_manager::PlatformManagerService>>
PmClientFactory::createSecureClient(
    const std::pair<std::string, std::string>& certAndKey) const {
  auto ctx = std::make_shared<folly::SSLContext>();
  ctx->loadCertificate(certAndKey.first.c_str());
  ctx->loadPrivateKey(certAndKey.second.c_str());
  ctx->setVerificationOption(
      folly::SSLContext::SSLVerifyPeerEnum::VERIFY_REQ_CLIENT_CERT);
  ctx->loadTrustedCertificates(facebook::services::kDefaultCaPath.data());
  // Pass "rs" protocol for ALPN to use RocketTransport during the TLS
  // handshake. Otherwise transport timeout.
  ctx->setAdvertisedNextProtocols({"rs"});
  folly::ssl::SSLCommonOptions::setClientOptions(*ctx);
  std::vector<facebook::infrasec::authorization::Identity> identities{
      infrasec::authorization::BaseIdentityUtil::makeIdentity(
          infrasec::authorization::acl_constants::MACHINE(),
          facebook::FbWhoAmI::getName())};
  auto verifier =
      std::make_shared<facebook::services::FacebookIdentityVerifier>(
          std::move(identities),
          facebook::services::IdentityVerificationMode::ANY,
          facebook::services::IdentityVerificationOptions{});
  auto socket = folly::AsyncSSLSocket::UniquePtr(new folly::AsyncSSLSocket(
      ctx,
      evb_.get(),
      folly::AsyncSSLSocket::Options{.verifier = std::move(verifier)}));
  socket->connect(/* ConnectCallback */ nullptr, pmSocketAddr_);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  return std::make_unique<
      apache::thrift::Client<platform_manager::PlatformManagerService>>(
      std::move(channel));
}

} // namespace facebook::fboss::platform::sensor_service
