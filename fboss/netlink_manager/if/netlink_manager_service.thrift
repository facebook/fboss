include "common/fb303/if/fb303.thrift"
include "fboss/agent/if/ctrl.thrift"

namespace py fboss.netlink_manager.netlink_manager_service
namespace cpp2 facebook.fboss

service NetlinkManagerService extends fb303.FacebookService {
  bool status();
  list<string> getMonitoredInterfaces();
  void addInterfaceToNetlinkManager(1: string ifname);
  void removeInterfaceFromNetlinkManager(1: string ifname);
  i32 addRoute(1: ctrl.UnicastRoute unicastRoute, 2: i16 family);
  i32 deleteRoute(1: ctrl.UnicastRoute unicastRoute, 2: i16 family);
}
