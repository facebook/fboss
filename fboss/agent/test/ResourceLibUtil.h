// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/IPAddressV4.h>
#include <folly/Singleton.h>
#include <cstdint>
#include <type_traits>

#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/Route.h"

namespace facebook {
namespace fboss {
namespace utility {

template <typename IdT>
class ResourceCursor {
 public:
  ResourceCursor();
  IdT getNext();
  void resetCursor(IdT count);
  IdT getCursorPosition() const;

 private:
  IdT current_;
};

template <typename AddrT>
struct IdT {
  using type = std::
      enable_if_t<std::is_same<AddrT, folly::IPAddressV4>::value, uint32_t>;
};

} // namespace utility
} // namespace fboss
} // namespace facebook
