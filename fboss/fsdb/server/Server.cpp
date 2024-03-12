// Copyright 2004-present Facebook. All Rights Reserved.

#include <boost/algorithm/string/join.hpp>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <folly/init/Init.h>

#include <folly/io/async/AsyncSignalHandler.h>
#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/server/FsdbConfig.h"
#include "fboss/fsdb/server/Server.h"
#include "fboss/fsdb/server/ServiceHandler.h"
#include "fboss/fsdb/server/ThriftAcceptor.h"

#include <signal.h>

using namespace std::chrono_literals; // @donotremove

DEFINE_bool(readConfigFile, true, "Whether config file should be read");

DEFINE_int32(
    streamExpire_ms,
    1 * 15 * 60 * 1000 /* quarter hour */,
    "Delay after which connection to a publisher that has not "
    "published to any metric is closed");

// NOTE: we create/open rocksdb instances per publisher before accepting
// connections from publishers (or subscribers). The delay incurred
// by rocksdb::DB::Open() is significant and may cause publisher
// connection to time out if invoked in the Thrift handler thread.
// We avoid that by creating/opening the rocksdb instance at startup.
// This has the downside of requiring the list of publishers to be available
// when FSDB starts. We use gflag (joined by comma) for this purpose
// (FSDB config can override).
DEFINE_string(
    publisherIdsToOpenRocksDbAtStartFor,
    "wedge_agent,qsfp_service,bgp,openr",
    "PublisherIds for which RocksDB instances will be created/opened"
    " when FSDB starts and before accepting any incoming connections");

DEFINE_int32(
    stat_publish_interval_ms,
    1000,
    "How frequently to publish thread-local stats back to the "
    "global store. This should generally be less than 1 second.");

DEFINE_bool(
    enable_thrift_acceptor,
    false,
    "whether to enable ThriftAcceptorFactory");

DEFINE_bool(
    enable_tos_reflect,
    false,
    "whether to enable ThriftServer ToS reflection");

DEFINE_string(
    ssl_policy,
    "required",
    "set SSL policy for thrift clients: disabled, permitted, required (default)");

DEFINE_string(
    trustedSubnets,
    "",
    "DSF cluster in-band subnets, separated by ','");

DEFINE_bool(
    useIdPathsForSubs,
    false,
    "Convert subscriber paths to id paths and serve only ids");

namespace facebook::fboss::fsdb {

static apache::thrift::SSLPolicy getThriftServerSSLPolicy() {
  if (FLAGS_ssl_policy == "disabled") {
    return apache::thrift::SSLPolicy::DISABLED;
  } else if (FLAGS_ssl_policy == "permitted") {
    return apache::thrift::SSLPolicy::PERMITTED;
  } else if (FLAGS_ssl_policy == "required") {
    return apache::thrift::SSLPolicy::REQUIRED;
  } else {
    XLOG(ERR) << "Invalid Thrift server sslPolicy: " << FLAGS_ssl_policy;
    exit(1);
  }
}

static std::vector<folly::CIDRNetwork> getTrustedSubnets(
    const std::vector<std::string>& trustedSubnetsFromConfig) {
  std::vector<std::string> subnetsList;
  // override the subnetsList from config if specified in command line flags
  if (FLAGS_trustedSubnets != "") {
    folly::split(',', FLAGS_trustedSubnets, subnetsList);
  } else {
    subnetsList = trustedSubnetsFromConfig;
  }
  std::vector<folly::CIDRNetwork> trustedSubnets;
  for (auto& subnet : subnetsList) {
    auto parsed = folly::IPAddress::createNetwork(subnet);
    trustedSubnets.push_back(parsed);
  }
  XLOG(INFO) << "trustedSubnets: " << boost::algorithm::join(subnetsList, ", ");
  return trustedSubnets;
}

std::shared_ptr<FsdbConfig> parseConfig(int argc, char** argv) {
  // one pass over flags, but don't clear argc/argv. We only do this
  // to extract the 'fsdb_config' arg.
  gflags::ParseCommandLineFlags(&argc, &argv, false);
  return FLAGS_readConfigFile ? FsdbConfig::fromDefaultFile()
                              : std::shared_ptr<FsdbConfig>();
}

void initFlagDefaults(
    const std::unordered_map<std::string, std::string>& defaults) {
  for (const auto& item : defaults) {
    // logging not initialized yet, need to use std::cerr
    std::cerr << "Overriding default flag from config: " << item.first.c_str()
              << "=" << item.second.c_str() << std::endl;
    gflags::SetCommandLineOptionWithMode(
        item.first.c_str(), item.second.c_str(), gflags::SET_FLAGS_DEFAULT);
  }
}

std::shared_ptr<ServiceHandler> createThriftHandler(
    std::shared_ptr<FsdbConfig> fsdbConfig) {
  return std::make_shared<ServiceHandler>(
      fsdbConfig,
      FLAGS_publisherIdsToOpenRocksDbAtStartFor,
      ServiceHandler::Options().setServeIdPathSubs(FLAGS_useIdPathsForSubs));
}

std::shared_ptr<apache::thrift::ThriftServer> createThriftServer(
    std::shared_ptr<FsdbConfig> fsdbConfig,
    std::shared_ptr<ServiceHandler> handler) {
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setInterface(handler);
  std::vector<folly::SocketAddress> addresses;
  for (auto port : {FLAGS_fsdbPort, FLAGS_migrated_fsdbPort}) {
    folly::SocketAddress address;
    address.setFromLocalPort(port);
    addresses.push_back(address);
  }
  server->setAddresses(addresses);
  server->setStreamExpireTime(
      std::chrono::milliseconds((FLAGS_streamExpire_ms)));
  server->setAllowPlaintextOnLoopback(true);
  server->setWorkersJoinTimeout(std::chrono::seconds(2));
  server->setQuickExitOnShutdownTimeout(true);
  server->setTosReflect(FLAGS_enable_tos_reflect);
  server->setSSLPolicy(getThriftServerSSLPolicy());
  if (FLAGS_enable_thrift_acceptor) {
    auto trustedSubnets =
        getTrustedSubnets(fsdbConfig->getThrift().get_trustedSubnets());
    server->setAcceptorFactory(
        std::make_shared<FsdbThriftAcceptorFactory<void>>(
            server.get(), std::nullopt, trustedSubnets));
  }
  return server;
}

} // namespace facebook::fboss::fsdb
