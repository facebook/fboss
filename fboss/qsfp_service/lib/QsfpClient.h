/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <map>
#include <memory>
#include <vector>

#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>

#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_clients.h"

namespace facebook {
namespace fboss {

// Create a custom client that doesn't use service router
// Service router has hard dependencies on smcc and configerator-proxy
// being in working order
// To avoid being unable to configure ports, use raw thrift until they remove
// these dependencies from their critical path
// TODO(ninasc): t13719924
// TODO(joseph5wu): T103506336 Replace QsfpClient by ThriftServiceClient
class QsfpClient {
 public:
  QsfpClient() {}
  ~QsfpClient() {}

  static folly::Future<std::unique_ptr<apache::thrift::Client<QsfpService>>>
  createClient(folly::EventBase* eb);

  static apache::thrift::RpcOptions getRpcOptions();
};

} // namespace fboss
} // namespace facebook
