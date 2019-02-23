#include "NetlinkPoller.h"
#include "folly/io/async/EventBase.h"

namespace facebook {
namespace fboss {

NetlinkPoller::NetlinkPoller(
    folly::EventBase* eb,
    int fd,
    struct nl_cache_mngr* manager)
    : folly::EventHandler(eb, folly::NetworkSocket::fromFd(fd)),
      manager_(manager) {
  DCHECK(eb) << "NULL pointer to EventBase";
  if (fd == -1) {
    int mngrFd = nl_cache_mngr_get_fd(manager);
    changeHandlerFD(folly::NetworkSocket::fromFd(mngrFd));
  }
  registerHandler(folly::EventHandler::READ | folly::EventHandler::PERSIST);
}

void NetlinkPoller::handlerReady(uint16_t /*events*/) noexcept {
  nl_cache_mngr_data_ready(manager_);
  return;
}
} // namespace fboss
} // namespace facebook
