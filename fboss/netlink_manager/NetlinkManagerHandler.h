#pragma once

extern "C" {
#include <netlink/route/route.h>
#include <netlink/socket.h>
#include <netlink/types.h>
#include <sys/socket.h>
}

#include <thread>
#include "common/fb303/cpp/FacebookBase2.h"
#include "fboss/netlink_manager/NetlinkManager.h"
#include "fboss/netlink_manager/if/gen-cpp2/NetlinkManagerService.h"

namespace folly {
class EventBase;
}

namespace facebook {
namespace fboss {

class NetlinkManagerHandler : public facebook::fboss::NetlinkManagerServiceSvIf,
                              public facebook::fb303::FacebookBase2 {
 public:
  NetlinkManagerHandler();
  bool status() override;
  void getMonitoredInterfaces(std::vector<std::string>& interfaces) override;
  void addInterfaceToNetlinkManager(
      std::unique_ptr<std::string> ifname) override;
  void removeInterfaceFromNetlinkManager(
      std::unique_ptr<std::string> ifname) override;
  // These 2 are for testing. They generate netlink messages
  int32_t addRoute(
      std::unique_ptr<::facebook::fboss::UnicastRoute> unicastRoute,
      int16_t family) override;
  int32_t deleteRoute(
      std::unique_ptr<::facebook::fboss::UnicastRoute> unicastRoute,
      int16_t family) override;
  void init();
  std::unique_ptr<std::thread> netlinkManagerThread_{nullptr};

 private:
  struct nl_sock* sock_{nullptr}; // pipe to RX/TX netlink messages
  std::unique_ptr<facebook::fboss::NetlinkManager> nlm_{nullptr};
  void connectToNetlink();

  int32_t updateRoute(
      const ::facebook::fboss::UnicastRoute& unicastRoute,
      int16_t family,
      int16_t nlOperation);
};
} // namespace fboss
} // namespace facebook
