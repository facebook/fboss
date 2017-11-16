#pragma once

extern "C" {
#include <netlink/route/addr.h>
#include <netlink/route/route.h>
// net/if.h has many similar definition with linux/if.h
#ifndef IFNAMSIZ
#define IFNAMSIZ 32
#endif
char* if_indextoname(unsigned int ifindex, char* ifname);
unsigned int if_nametoindex(const char* ifname);
}

#include "common/network/if/gen-cpp2/Address_types.h"

namespace folly {
class IPAddress;
}
namespace facebook {
namespace fboss {

typedef facebook::network::thrift::BinaryAddress BinaryAddress;
class IpPrefix;
class UnicastRoute;

folly::IPAddress nlAddrToFollyAddr(struct nl_addr* addr);
BinaryAddress nlAddrToBinAddr(struct nl_addr* addr);
IpPrefix nlAddrToIpPrefix(struct nl_addr* addr);
std::string binAddrToStr(BinaryAddress& addr);
struct nl_addr* binAddrToNlAddr(BinaryAddress bin_addr, int16_t family);
void unicastRouteToRtnlRoute(
    UnicastRoute unicastRoute,
    int family,
    struct rtnl_route* route);

std::vector<BinaryAddress> getNextHops(
    struct rtnl_route* route,
    int ipLen,
    const std::set<std::string>& interfaceNames);

struct nl_addr* getNlGatewayFromNlRoute(
    struct rtnl_route* route,
    int i,
    const std::set<std::string>& interfaceNames,
    int ipLen);
} // namespace fboss
} // namespace facebook
