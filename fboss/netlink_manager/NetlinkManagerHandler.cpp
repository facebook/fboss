#include "NetlinkManagerHandler.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/netlink_manager/utils/AddressUtils.h"
#include "folly/ScopeGuard.h"
#include "folly/io/async/EventBase.h"
#include "thrift/lib/cpp/TApplicationException.h"

namespace facebook {
namespace fboss {

NetlinkManagerHandler::NetlinkManagerHandler()
    : FacebookBase2("Netlink Manager Service") {}

bool NetlinkManagerHandler::status() {
  return true;
}

void NetlinkManagerHandler::init() {
  netlinkManagerThread_ = std::make_unique<std::thread>([this] {
    folly::EventBase eb;
    nlm_ = std::make_unique<facebook::fboss::NetlinkManager>(&eb);
    nlm_->run();
    eb.loopForever();
  });
}

void NetlinkManagerHandler::getMonitoredInterfaces(
    std::vector<std::string>& interfaces) {
  std::set<std::string> monitored_interfaces = nlm_->getMonitoredInterfaces();
  for (std::string interface : monitored_interfaces) {
    interfaces.emplace_back(interface);
  }
}

void NetlinkManagerHandler::addInterfaceToNetlinkManager(
    std::unique_ptr<std::string> ifname) {
  nlm_->startMonitoringInterface(*ifname);
  return;
}

void NetlinkManagerHandler::removeInterfaceFromNetlinkManager(
    std::unique_ptr<std::string> ifname) {
  nlm_->stopMonitoringInterface(*ifname);
  return;
}

int32_t NetlinkManagerHandler::addRoute(
    std::unique_ptr<::facebook::fboss::UnicastRoute> unicastRoute,
    int16_t family) {
  return updateRoute(*unicastRoute, family, NL_ACT_NEW);
}

int32_t NetlinkManagerHandler::deleteRoute(
    std::unique_ptr<::facebook::fboss::UnicastRoute> unicastRoute,
    int16_t family) {
  return updateRoute(*unicastRoute, family, NL_ACT_DEL);
}

int32_t NetlinkManagerHandler::updateRoute(
    const ::facebook::fboss::UnicastRoute& unicastRoute,
    int16_t family,
    int16_t nlOperation) {
  int errCode = 0;
  std::string err_msg;

  struct rtnl_route* route = rtnl_route_alloc();
  try {
    unicastRouteToRtnlRoute(unicastRoute, family, route);
  } catch (const std::runtime_error& e) {
    VLOG(0) << "Error converting unicast route to rtnl_route " << e.what();
    return -1;
  }
  SCOPE_EXIT {
    rtnl_route_put(route);
  };

  struct nl_sock* sock = nlm_->getNetlinkSocket();
  switch (nlOperation) {
    case NL_ACT_NEW: {
      errCode = rtnl_route_add(sock, route, 0);
      if (errCode < 0) {
        err_msg = std::string(nl_geterror(errCode));
        VLOG(0) << unicastRoute.dest.ip.ifName;
        VLOG(0) << "rtnl_route_add failed. Error message: " << err_msg;
      }
      break;
    }
    case NL_ACT_DEL: {
      errCode = rtnl_route_delete(sock, route, 0);
      if (errCode < 0) {
        err_msg = std::string(nl_geterror(errCode));
        VLOG(0) << "rtnl_route_delete failed. Error message: " << err_msg;
      }
      break;
    }
    default: {
      VLOG(0) << "Not updating state due to unknown netlink operation "
              << std::to_string(nlOperation);
      break; /* NL_ACT_??? */
    }
  }

  return errCode;
}
} // namespace fboss
} // namespace facebook
