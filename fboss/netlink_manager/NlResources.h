#pragma once

extern "C" {
#include <netlink/route/route.h>
#include <netlink/socket.h>
}

namespace facebook {
namespace fboss {

struct NlResources {
  NlResources();
  ~NlResources();

  struct nl_sock* sock; // pipe to RX/TX netlink messages
  struct nl_cache* routeCache; // our copy of the system route state
  struct nl_cache_mngr* manager; // wraps caches and notifies upon a change
};
} // namespace fboss
} // namespace facebook
