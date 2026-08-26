// Copyright 2004-present Facebook. All Rights Reserved.
//
//
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/InitCli.h"
#include "fboss/platform/helpers/PlatformThriftAcceptor.h"
#include "fboss/platform/helpers/PlatformThriftAcceptorUtil.h"

#include <folly/init/Init.h>
#include <gflags/gflags.h>

#include "fboss/lib/ThriftServiceUtils.h"

DEFINE_bool(
    platform_enable_thrift_acceptor,
    false,
    "If set, install a connection-level acceptor that admits Thrift "
    "connections only from loopback or --platform_trusted_subnets and rejects "
    "all others. Off by default to preserve existing OSS behavior.");

DEFINE_string(
    platform_trusted_subnets,
    "",
    "Comma-separated CIDR subnets, in addition to loopback (which is always "
    "permitted), allowed to connect when --platform_enable_thrift_acceptor is "
    "set. FBOSS control traffic is IPv6-only; e.g. \"2001:db8::/32\".");

namespace facebook::fboss::platform::helpers {

void init(int* argc, char*** argv) {
  folly::init(argc, argv, true);
}

void initCli(int* argc, char*** argv, const std::string&) {
  folly::init(argc, argv, true);
}

std::string getBuildVersion() {
  return "Not implemented";
}

std::string getBuildSummary() {
  return "Not implemented";
}

void runThriftService(
    std::shared_ptr<apache::thrift::ThriftServer> server,
    std::shared_ptr<apache::thrift::ServerInterface> handler,
    const std::string& /* serviceName */,
    uint32_t port) {
  // Setup thrift server
  facebook::fboss::ThriftServiceUtils::setPreferredEventBaseBackend(*server);
  server->setPort(port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);

  auto evb = server->getEventBaseManager()->getEventBase();
  SignalHandler signalHandler(evb, server);

  if (FLAGS_platform_enable_thrift_acceptor) {
    auto trustedSubnets = parseTrustedSubnets(FLAGS_platform_trusted_subnets);
    XLOG(INFO) << "Thrift connection acceptor enabled: admitting loopback + "
               << trustedSubnets.size() << " trusted subnet(s)";
    server->setAcceptorFactory(
        std::make_shared<PlatformThriftAcceptorFactory>(
            server.get(), std::move(trustedSubnets)));
  }

  server->serve();
}

} // namespace facebook::fboss::platform::helpers
