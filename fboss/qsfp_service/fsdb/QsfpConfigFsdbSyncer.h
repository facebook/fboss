/*
 *  Copyright (c) 2021-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/fsdb/client/FsdbBaseComponentSyncer.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "folly/futures/Future.h"
#include "folly/futures/Promise.h"

namespace facebook::fboss {

class QsfpConfigFsdbSyncer
    : public fsdb::FsdbStateComponentSyncer<cfg::QsfpServiceConfig> {
 public:
  explicit QsfpConfigFsdbSyncer(folly::EventBase* evb);

  cfg::QsfpServiceConfig getCurrentState() override;
  void configUpdated(const cfg::QsfpServiceConfig& newConfig);
  folly::SemiFuture<folly::Unit> dataReady();

 private:
  void publishPendingConfig();

  folly::Synchronized<std::unique_ptr<cfg::QsfpServiceConfig>> publishedConfig_;
  folly::Synchronized<std::unique_ptr<cfg::QsfpServiceConfig>> pendingConfig_;
  folly::Promise<folly::Unit> dataReady_;
};

} // namespace facebook::fboss
