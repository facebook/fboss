#include "AddressUtils.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "folly/FBString.h"
#include "folly/IPAddress.h"

namespace facebook {
namespace fboss {

folly::IPAddress nlAddrToFollyAddr(struct nl_addr* addr) {
  return folly::IPAddress::fromBinary(folly::ByteRange(
      reinterpret_cast<const unsigned char*>(nl_addr_get_binary_addr(addr)),
      nl_addr_get_len(addr)));
}

IpPrefix nlAddrToIpPrefix(struct nl_addr* addr) {
  IpPrefix prefix;
  prefix.ip = nlAddrToBinAddr(addr);
  prefix.prefixLength = nl_addr_get_prefixlen(addr);
  return prefix;
}

BinaryAddress nlAddrToBinAddr(struct nl_addr* addr) {
  BinaryAddress binAddr;
  binAddr.addr = folly::fbstring(
      (char*)nl_addr_get_binary_addr(addr), nl_addr_get_len(addr));
  return binAddr;
}

std::string binAddrToStr(BinaryAddress& addr) {
  const unsigned char* binaryData =
      reinterpret_cast<const unsigned char*>(addr.addr.data());
  folly::IPAddress ipAddr = folly::IPAddress::fromBinary(
      folly::ByteRange(binaryData, addr.addr.length()));
  return ipAddr.str();
}

struct nl_addr* binAddrToNlAddr(BinaryAddress binAddr, int16_t family) {
  struct nl_addr* nlAddr =
      nl_addr_build(family, (void*)(binAddr.addr.data()), binAddr.addr.size());
  if (nlAddr == nullptr) {
    throw std::runtime_error("can't build nl addr");
  }
  // TODO: handle this exception here?
  return nlAddr;
}

void unicastRouteToRtnlRoute(
    UnicastRoute unicastRoute,
    int family,
    struct rtnl_route* route) {
  if (route == nullptr) {
    throw std::runtime_error("Cannot allocate route object");
  }
  struct nl_addr* nlDst = binAddrToNlAddr(unicastRoute.dest.ip, family);
  SCOPE_EXIT {
    nl_addr_put(nlDst);
  };

  nl_addr_set_prefixlen(nlDst, unicastRoute.dest.prefixLength);
  auto errCode = rtnl_route_set_dst(route, nlDst);
  if (errCode != 0) {
    throw std::runtime_error("cannot set destination to nl_route obj");
  }

  struct rtnl_nexthop* nextHop = rtnl_route_nh_alloc();
  unsigned int ifindex = if_nametoindex(unicastRoute.dest.ip.ifName.c_str());
  struct nl_addr* nlGateway;
  for (auto const& binaryNexthop : unicastRoute.nextHopAddrs) {
    nlGateway = binAddrToNlAddr(binaryNexthop, family);
    rtnl_route_nh_set_ifindex(nextHop, ifindex);
    rtnl_route_nh_set_gateway(nextHop, nlGateway);
    rtnl_route_add_nexthop(route, nextHop);
  }
  return;
}

std::vector<BinaryAddress> getNextHops(
    struct rtnl_route* route,
    int ipLen,
    const std::set<std::string>& interfaceNames) {
  std::vector<BinaryAddress> nexthops;

  int numNexthops = rtnl_route_get_nnexthops(route);
  for (int i = 0; i < numNexthops; i++) {
    auto nlAddrGateway =
        getNlGatewayFromNlRoute(route, i, interfaceNames, ipLen);
    nexthops.push_back(nlAddrToBinAddr(nlAddrGateway));
  }

  return nexthops;
}

struct nl_addr* getNlGatewayFromNlRoute(
    struct rtnl_route* route,
    int i,
    const std::set<std::string>& interfaceNames,
    int ipLen) {
  struct nl_addr* nlAddrGateway = nullptr;
  struct rtnl_nexthop* nh = rtnl_route_nexthop_n(route, i);
  int ifindex = rtnl_route_nh_get_ifindex(nh);
  char ifname[IFNAMSIZ];
  if_indextoname(ifindex, ifname);
  if (interfaceNames.find(ifname) == interfaceNames.end()) {
    VLOG(1) << "Interface index  " << ifindex << "is not set to be monitored";
  }
  nlAddrGateway = rtnl_route_nh_get_gateway(nh);
  char strGateway[ipLen];
  nl_addr2str(nlAddrGateway, strGateway, ipLen);
  if (strcmp("none", strGateway) == 0) {
    VLOG(1) << "Route update has no gateway";
  }
  return nlAddrGateway;
}
} // namespace fboss
} // namespace facebook
