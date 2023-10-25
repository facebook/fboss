// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <cstdint>
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

namespace folly {
class EventBase;
}

namespace facebook::fboss::fsdb {

class Client {
 public:
  static std::unique_ptr<apache::thrift::Client<FsdbService>> getClient(
      const folly::SocketAddress& dstAddr,
      const std::optional<folly::SocketAddress>& srcAddr,
      std::optional<uint8_t> tos,
      const bool plaintextClient,
      folly::EventBase* eb);
};

} // namespace facebook::fboss::fsdb
