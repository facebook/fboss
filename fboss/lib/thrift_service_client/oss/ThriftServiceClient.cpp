/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/thrift_service_client/ThriftServiceClient.h"
#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"

namespace facebook::fboss::utils {

template <typename ClientT>
std::unique_ptr<apache::thrift::Client<ClientT>> tryCreateEncryptedClient(
    const ConnectionOptions& options,
    folly::EventBase* eb) {
  return createPlaintextClient<ClientT>(std::move(options), eb);
}

template std::unique_ptr<apache::thrift::Client<facebook::fboss::FbossCtrl>>
tryCreateEncryptedClient(
    const ConnectionOptions& options,
    folly::EventBase* eb);
template std::unique_ptr<apache::thrift::Client<facebook::fboss::QsfpService>>
tryCreateEncryptedClient(
    const ConnectionOptions& options,
    folly::EventBase* eb);
template std::unique_ptr<
    apache::thrift::Client<facebook::fboss::fsdb::FsdbService>>
tryCreateEncryptedClient(
    const ConnectionOptions& options,
    folly::EventBase* eb);
} // namespace facebook::fboss::utils
