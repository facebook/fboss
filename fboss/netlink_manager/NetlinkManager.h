#pragma once

extern "C" {
#include <netlink/route/route.h>
#include <netlink/socket.h>
}

#include <mutex>
#include <string>
#include "NetlinkPoller.h"
#include "NlResources.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/netlink_manager/utils/AddressUtils.h"

namespace folly {
class EventBase;
}
namespace {
struct nl_dump_params initDumpParams();
}
namespace facebook {
namespace fboss {
using FbossClient = std::unique_ptr<FbossCtrlAsyncClient>;

class NetlinkManager {
 public:
  const static int FBOSS_CLIENT_ID =
      static_cast<int>(StdClientIds::NETLINK_LISTENER);
  NetlinkManager(folly::EventBase* eb);
  void run();
  struct nl_sock* getNetlinkSocket() const;
  std::set<std::string> getMonitoredInterfaces();
  void startMonitoringInterface(std::string ifname);
  void stopMonitoringInterface(std::string ifname);
  static void checkError(int errCode, std::string messages);

 private:
  void setMonitoredInterfaces();
  std::set<std::string> getFbossInterfaces();
  std::set<std::string> parseInterfacesArg(std::string interfacesStr);
  void registerWNetlink();
  folly::Future<FbossClient> getFbossClient(
      const std::string& ip,
      const int& port);
  void startListening();
  void testFbossClient();
  static void netlinkRouteUpdated(
      struct nl_cache* cache, // Route cache
      struct nl_object* obj, // New route object
      int nlOperation, // Type of operation (new, delete etc)
      void* data); // NetlinkManager itself
  inline std::string nlOperationToStr(int nlOperation) {
    switch (nlOperation) {
      case NL_ACT_UNSPEC:
        return "UNSPECIFIED";
      case NL_ACT_NEW:
        return "NEW";
      case NL_ACT_DEL:
        return "DELETE";
      case NL_ACT_CHANGE:
        return "CHANGE";
      case NL_ACT_SET:
        return "SET";
      case NL_ACT_GET:
        return "GET";
      default:
        return "unknown";
    }
  }
  void addRouteViaFbossThrift(
      struct nl_addr* nlDst,
      const std::vector<BinaryAddress>& nexthops);
  void deleteRouteViaFbossThrift(struct nl_addr* nlDst);
  void logAndDie(const char* msg);
  void terminateEventBase();

  std::set<std::string> monitoredInterfaces_;
  folly::EventBase* eb_;
  FbossClient fbossClient_{nullptr};
  std::unique_ptr<NetlinkPoller> poller_{nullptr};
  std::unique_ptr<NlResources> nlResources_{nullptr};
  std::mutex interfacesMutex_;
};
} // namespace fboss
} // namespace facebook
