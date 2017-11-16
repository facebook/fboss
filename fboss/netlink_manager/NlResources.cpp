#include "NlResources.h"
#include "NetlinkManager.h"
#include "NetlinkManagerException.h"

namespace facebook {
namespace fboss {

NlResources::NlResources() {
  sock = nl_socket_alloc();
  if (!sock) {
    throw NetlinkManagerException("Opened netlink socket failed");
  }
  VLOG(1) << "Opened netlink socket";

  auto errCode = nl_connect(sock, NETLINK_ROUTE);
  NetlinkManager::checkError(errCode, "Connecting to netlink socket failed");
  VLOG(1) << "Connected to netlink socket";

  errCode = rtnl_route_alloc_cache(sock, AF_UNSPEC, 0, &routeCache);
  NetlinkManager::checkError(errCode, "Allocating route cache failed");
  VLOG(1) << "Allocated route cache";

  errCode = nl_cache_mngr_alloc(nullptr, AF_UNSPEC, 0, &manager);
  NetlinkManager::checkError(errCode, "Allocating cache manager failed");
  VLOG(1) << "Allocated cache manager";

  nl_cache_mngt_provide(routeCache);
}

NlResources::~NlResources() {
  if (sock) {
    nl_socket_free(sock);
  }
  if (routeCache) {
    nl_cache_free(routeCache);
  }
  if (manager) {
    nl_cache_mngr_free(manager);
  }
}
} // namespace fboss
} // namespace facebook
