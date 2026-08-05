// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/qsfp_service/QsfpServer.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "fboss/lib/ThriftServiceUtils.h"
#include "fboss/platform/helpers/PlatformThriftAcceptor.h"
#include "fboss/platform/helpers/PlatformThriftAcceptorUtil.h"
#include "fboss/qsfp_service/QsfpServiceHandler.h"
#include "fboss/qsfp_service/platforms/wedge/FbossMacsecHandler.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

DEFINE_int32(port, 5910, "Port for the thrift service");
// current default 5910 is in conflict with VNC ports, need to
// eventually migrate to 5960
DEFINE_int32(migrated_port, 5960, "New thrift server port migrate to");

DEFINE_bool(
    qsfp_enable_thrift_acceptor,
    false,
    "If set, install a connection-level acceptor that admits Thrift "
    "connections only from loopback or --qsfp_trusted_subnets and rejects all "
    "others. Off by default to preserve existing behavior.");

DEFINE_string(
    qsfp_trusted_subnets,
    "",
    "Comma-separated CIDR subnets, in addition to loopback (which is always "
    "permitted), allowed to connect when --qsfp_enable_thrift_acceptor is set. "
    "FBOSS control traffic is IPv6-only; e.g. \"2001:db8::/32\".");

namespace facebook::fboss {

std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<QsfpServiceHandler>>
setupThriftServer(
    std::unique_ptr<WedgeManager> tcvrManager,
    std::unique_ptr<PortManager> portManager) {
  // Create Platform specific FbossMacsecHandler object
  auto macsecHandler =
      createFbossMacsecHandler(portManager.get(), tcvrManager.get());

  auto handler = std::make_shared<QsfpServiceHandler>(
      std::move(tcvrManager), std::move(portManager), macsecHandler);
  handler->init();
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  ThriftServiceUtils::setPreferredEventBaseBackend(*server);
  // Since thrift calls may involve programming HW, don't
  // set queue timeouts
  server->setQueueTimeout(std::chrono::milliseconds(0));
  server->setSocketQueueTimeout(std::chrono::milliseconds(0));
  // If a thrift request is in-flight during shutdown, ThriftServer will abort
  // (SIGABRT) if it can't drain within the join timeout. The default is 20
  // seconds which is too short when transceiver programming operations are
  // running. Set to 140 seconds (> process_wrapper's 120 second SIGABRT
  // timeout) so the ThriftServer never hits its own join deadline first.
  // process_wrapper's SIGABRT at 120s is the expected termination path for a
  // slow shutdown.
  server->setWorkersJoinTimeout(std::chrono::seconds(140));

  std::vector<folly::SocketAddress> addresses;
  for (auto port : {FLAGS_port, FLAGS_migrated_port}) {
    folly::SocketAddress address;
    address.setFromLocalPort(port);
    addresses.push_back(address);
  }
  server->setAddresses(addresses);
  server->setInterface(handler);

  // MACsec SAK install/delete and other qsfp_service RPCs are otherwise
  // unauthenticated on a wildcard-bound port. When enabled, admit only
  // loopback (where the on-box MKA daemon, fboss2, wedge_qsfp_util connect)
  // plus configured trusted subnets, rejecting off-box peers at accept time.
  if (FLAGS_qsfp_enable_thrift_acceptor) {
    auto trustedSubnets =
        platform::helpers::parseTrustedSubnets(FLAGS_qsfp_trusted_subnets);
    XLOG(INFO) << "Thrift connection acceptor enabled: admitting loopback + "
               << trustedSubnets.size() << " trusted subnet(s)";
    server->setAcceptorFactory(
        std::make_shared<platform::helpers::PlatformThriftAcceptorFactory>(
            server.get(), std::move(trustedSubnets)));
  }

  return std::make_pair(server, handler);
}

void initFlagDefaultsFromQsfpConfig() {
  auto qsfpConfig = QsfpConfig::fromDefaultFile();
  for (auto item : *qsfpConfig->thrift.defaultCommandLineArgs()) {
    // logging not initialized yet, need to use std::cerr
    std::cerr << "Overriding default flag from config: " << item.first.c_str()
              << "=" << item.second.c_str() << std::endl;
    gflags::SetCommandLineOptionWithMode(
        item.first.c_str(), item.second.c_str(), gflags::SET_FLAGS_DEFAULT);
  }
}
} // namespace facebook::fboss
