#include "NetlinkManager.h"
#include <chrono>
#include "NetlinkManagerException.h"
#include "folly/Format.h"
#include "folly/ScopeGuard.h"
#include "folly/futures/Future.h"
#include "folly/io/async/EventBase.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"
#include "thrift/lib/cpp2/async/HeaderClientChannel.h"

using facebook::fb303::cpp2::fb_status;

DEFINE_string(ip, "::1", "ip of the switch. Default to local host");
DEFINE_int32(fboss_port, 5909, "Port for the fboss ctrl thrift service");
DEFINE_bool(debug, false, "Enable debug mode");
DEFINE_string(
    interfaces,
    "",
    "comma-separated list of names of interfaces to listen to");

namespace {
struct nl_dump_params initDumpParams() {
  struct nl_dump_params dumpParams;
  memset(&dumpParams, 0, sizeof(dumpParams));
  dumpParams.dp_type = NL_DUMP_STATS;
  dumpParams.dp_fd = stdout;
  return dumpParams;
}
} // namespace
namespace facebook {
namespace fboss {

NetlinkManager::NetlinkManager(folly::EventBase* eb)
    : eb_(eb), nlResources_(nullptr) {
  VLOG(0) << "Constructing NetlinkManager ";
}

void NetlinkManager::run() {
  setMonitoredInterfaces();
  testFbossClient();
  callSyncFib();
  registerWNetlink();
  startListening();
}

struct nl_sock* NetlinkManager::getNetlinkSocket() const {
  return nlResources_->sock;
}

std::set<std::string> NetlinkManager::getMonitoredInterfaces() {
  std::lock_guard<std::mutex> lock(interfacesMutex_);
  return monitoredInterfaces_;
}

void NetlinkManager::startMonitoringInterface(std::string ifname) {
  std::lock_guard<std::mutex> lock(interfacesMutex_);
  monitoredInterfaces_.insert(ifname);
  return;
}

void NetlinkManager::stopMonitoringInterface(std::string ifname) {
  std::lock_guard<std::mutex> lock(interfacesMutex_);
  monitoredInterfaces_.erase(ifname);
  return;
}

// Creating an async socket needs to run on the thread you expect to handle the
// socket later. If not, there might be a race condition where requests came
// before the socket exists.
FbossClient NetlinkManager::getFbossClient(
    const std::string& ip,
    const int& port) {
  folly::SocketAddress addr(ip, port);
  auto socket =
      apache::thrift::async::TAsyncSocket::newSocket(eb_, addr, 2000);
  socket->setSendTimeout(5000);
  auto channel = apache::thrift::HeaderClientChannel::newChannel(socket);
  return std::make_unique<FbossCtrlAsyncClient>(std::move(channel));
}

void NetlinkManager::testFbossClient() {
  int triesLeft = 5;
  // We are retrying multiple times here to mitigate a race condition during
  // start-up time when fboss might not be up as fast as the netlink manager
  FbossClient fbossClient = getFbossClient(FLAGS_ip, FLAGS_fboss_port);
  while (triesLeft > 0) {
    try {
      fb_status s = fbossClient->sync_getStatus();
      if (s == fb_status::ALIVE) {
        break;
      } else {
        VLOG(0) << "Fboss client not alive. Retrying.";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        triesLeft--;
      }
    } catch (const std::exception& ex) {
      VLOG(0) << "Failed to ping fboss client. Retrying. Exception: "
              << ex.what();
      std::this_thread::sleep_for(std::chrono::seconds(1));
      triesLeft--;
    }
  }
  if (triesLeft > 0) {
    return;
  } else {
    logAndDie("Failed to ping fboss client.");
  }
}

// The fboss agent will not accept incremental route changes
// (e.g., addUnicastRoute() ) until *after* syncFib() is called
//  to unblock the open source, it's probably ok to call syncFib()
// with an empty list of routes and then we'll go back and fix this.
void NetlinkManager::callSyncFib() {
  std::vector<UnicastRoute> routes;
  FbossClient fbossClient = getFbossClient(FLAGS_ip, FLAGS_fboss_port);
  fbossClient->sync_syncFib(FBOSS_CLIENT_ID, routes);
}

void NetlinkManager::setMonitoredInterfaces() {
  std::lock_guard<std::mutex> lock(interfacesMutex_);
  if (FLAGS_interfaces == "") {
    monitoredInterfaces_ = getFbossInterfaces();
  } else {
    monitoredInterfaces_ = parseInterfacesArg(FLAGS_interfaces);
  }
}

std::set<std::string> NetlinkManager::getFbossInterfaces() {
  std::set<std::string> interfaceNames;
  std::map<int32_t, fboss::InterfaceDetail> interfaceDetails;
  // getAllInterfaces only get Fboss interfaces, not eth0, lo, etc
  FbossClient fbossClient = getFbossClient(FLAGS_ip, FLAGS_fboss_port);
  fbossClient->sync_getAllInterfaces(interfaceDetails);
  for (auto const& interfaceDetail : interfaceDetails) {
    // get_interfaceName() returns "Interface 10", while we want "fboss10"
    // for if_nametoindex so I'm going this roundabout way.
    int fbossInterfaceId = interfaceDetail.second.get_interfaceId();
    std::string interfaceName = "fboss" + std::to_string(fbossInterfaceId);
    interfaceNames.insert(interfaceName);
  }
  return interfaceNames;
}

std::set<std::string> NetlinkManager::parseInterfacesArg(
    std::string interfacesStr) {
  std::vector<std::string> vectorInterfaces;
  folly::split(",", interfacesStr, vectorInterfaces);
  return std::set<std::string>(
      vectorInterfaces.begin(), vectorInterfaces.end());
}

void NetlinkManager::registerWNetlink() {
  initDumpParams();

  nlResources_ = std::make_unique<NlResources>();
  auto errCode = nl_cache_mngr_add_cache(
      nlResources_->manager,
      nlResources_->routeCache,
      netlinkRouteUpdated,
      this);
  checkError(errCode, "Failed to add route cache to cache manager");
  VLOG(1) << "Added route cache to cache manager";
  return;
}

void NetlinkManager::startListening() {
  int fd = nl_cache_mngr_get_fd(nlResources_->manager);
  poller_ = std::make_unique<NetlinkPoller>(eb_, fd, nlResources_->manager);
}

void NetlinkManager::terminateEventBase() {
  eb_->terminateLoopSoon();
}

void NetlinkManager::logAndDie(const char* msg) {
  VLOG(0) << std::string(msg);
  terminateEventBase();
  exit(1);
}

void NetlinkManager::checkError(int errCode, std::string message) {
  if (errCode < 0) {
    throw NetlinkManagerException(
        folly::sformat("{}. Error message: {}", message, nl_geterror(errCode)));
  }
}

void NetlinkManager::netlinkRouteUpdated(
    struct nl_cache* /*cache*/,
    struct nl_object* obj,
    int nlOperation,
    void* data) {
  NetlinkManager* nlm = static_cast<NetlinkManager*>(data);
  std::string operation = nlm->nlOperationToStr(nlOperation);
  if (operation == "unknown") {
    VLOG(1) << "Ignoring an unknown route update";
    return;
  }

  VLOG(2) << "Received a " << operation << " netlink route update message";
  struct rtnl_route* route = (struct rtnl_route*)obj;

  if (rtnl_route_get_type(route) != RTN_UNICAST) {
    VLOG(1) << "Ignoring non-unicast route update";
    return;
  }

  struct nl_addr* nlDst = rtnl_route_get_dst(route);
  const uint8_t ipLen = nl_addr_get_prefixlen(nlDst);
  char strDst[ipLen];
  nl_addr2str(nlDst, strDst, ipLen);

  int numNexthops = rtnl_route_get_nnexthops(route);
  if (!numNexthops) {
    VLOG(0) << "Could not find next hop for route update for " << strDst;
    struct nl_dump_params dumpParams = initDumpParams();
    nl_object_dump((nl_object*)route, &dumpParams);
    return;
  }

  std::vector<BinaryAddress> nexthops;
  {
    std::lock_guard<std::mutex> lock(nlm->interfacesMutex_);
    nexthops = getNextHops(route, ipLen, nlm->monitoredInterfaces_);
  }

  if (nexthops.empty()) {
    VLOG(1) << operation << " Route update for " << strDst
            << " has no valid nexthop";
    return;
  }

  if (FLAGS_debug) {
    VLOG(1) << "Got " << operation << " route update for " << strDst;
    return;
  }

  switch (nlOperation) {
    case NL_ACT_NEW: {
      nlm->addRouteViaFbossThrift(nlDst, nexthops);
      break;
    }
    case NL_ACT_DEL: {
      nlm->deleteRouteViaFbossThrift(nlDst);
      break;
    }
    case NL_ACT_CHANGE: {
      VLOG(2) << "Not updating state due to unimplemented"
              << "NL_ACT_CHANGE netlink operation";
      break;
    }
    default: {
      VLOG(0) << "Not updating state due to unknown netlink operation "
              << std::to_string(nlOperation);
      break; /* NL_ACT_??? */
    }
  }
  return;
}

void NetlinkManager::addRouteViaFbossThrift(
    struct nl_addr* nlDst,
    const std::vector<BinaryAddress>& nexthops) {
  UnicastRoute unicastRoute;
  unicastRoute.dest = nlAddrToIpPrefix(nlDst);
  unicastRoute.nextHopAddrs = nexthops;
  eb_->runInEventBaseThread([this, unicastRoute]() {
    FbossClient fbossClient = getFbossClient(FLAGS_ip, FLAGS_fboss_port);
    fbossClient->future_addUnicastRoute(FBOSS_CLIENT_ID, unicastRoute)
        .then([]() { VLOG(2) << "NetlinkManager route add success"; })
        .onError([unicastRoute](const std::exception& ex) {
          BinaryAddress binaryDst = unicastRoute.dest.ip;
          VLOG(2) << folly::sformat(
              "Route add failed for destination {}. Error sending thrift calls to FBOSS agent: {}",
              binAddrToStr(binaryDst),
              ex.what());
        });
  });
}

void NetlinkManager::deleteRouteViaFbossThrift(struct nl_addr* nlDst) {
  IpPrefix prefix = nlAddrToIpPrefix(nlDst);

  eb_->runInEventBaseThread([this, prefix]() {
    FbossClient fbossClient = getFbossClient(FLAGS_ip, FLAGS_fboss_port);
    fbossClient->future_deleteUnicastRoute(FBOSS_CLIENT_ID, prefix)
        .then([]() { VLOG(2) << "NetlinkManager route delete success"; })
        .onError([prefix](const std::exception& ex) {
          BinaryAddress binaryDst = prefix.ip;
          VLOG(2) << folly::sformat(
              "Route delete failed for destination {}. Error sending thrift calls to FBOSS agent: {}",
              binAddrToStr(binaryDst),
              ex.what());
        });
  });
}
} // namespace fboss
} // namespace facebook
