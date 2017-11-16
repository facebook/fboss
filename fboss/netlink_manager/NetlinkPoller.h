extern "C" {
#include <netlink/cache.h>
}

#include <folly/io/async/EventHandler.h>

namespace folly {
class EventBase;
}

namespace facebook {
namespace fboss {

/*
 * This class listen in on the netlink socket and call handlerReady when an
 * event is registered.
 */

class NetlinkPoller : public folly::EventHandler {
 public:
  NetlinkPoller(folly::EventBase* eb, int fd, struct nl_cache_mngr* manager);
  void handlerReady(uint16_t events) noexcept override;

 private:
  struct nl_cache_mngr* manager_;
};

} // namespace fboss
} // namespace facebook
