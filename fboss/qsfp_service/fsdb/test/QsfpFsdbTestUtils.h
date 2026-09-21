// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gtest/gtest.h>

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/DebugProtocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types_custom_protocol.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_state_types_custom_protocol.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_stats_types_custom_protocol.h"

namespace facebook::fboss {

namespace cfg {
inline std::ostream& operator<<(
    std::ostream& os,
    const std::optional<QsfpServiceConfig>& config) {
  return os
      << (config.has_value() ? apache::thrift::debugString(config.value())
                             : "<unset>");
}
} // namespace cfg

namespace stats {
inline std::ostream& operator<<(
    std::ostream& os,
    const std::optional<QsfpStats>& stat) {
  return os
      << (stat.has_value() ? apache::thrift::debugString(stat.value())
                           : "<unset>");
}
} // namespace stats

namespace state {
inline std::ostream& operator<<(
    std::ostream& os,
    const std::optional<QsfpState>& state) {
  return os
      << (state.has_value() ? apache::thrift::debugString(state.value())
                            : "<unset>");
}
} // namespace state

namespace phy {
inline std::ostream& operator<<(std::ostream& os, const PhyState& state) {
  return os << apache::thrift::debugString(state);
}

inline std::ostream& operator<<(std::ostream& os, const PhyStats& stats) {
  return os << apache::thrift::debugString(stats);
}

} // namespace phy

template <typename Path>
class TestSubscription {
 public:
  TestSubscription(
      fsdb::FsdbPubSubManager* pubSubManager,
      const Path& path,
      bool isStat = false)
      : pubSubManager_(pubSubManager),
        path_(path.tokens()),
        pathStr_(path.str()),
        isStat_(isStat) {
    auto streamStateChange = [](fsdb::SubscriptionState /* old */,
                                fsdb::SubscriptionState /* new */,
                                std::optional<bool> /*initialSyncHasData*/) {};
    auto dataUpdate = [&](fsdb::OperState state) {
      data_.withWLock([&](auto& locked) {
        if (auto contents = state.contents()) {
          locked = apache::thrift::BinarySerializer::deserialize<
              typename Path::DataT>(*contents);
          XLOG(INFO) << "Received update for " << pathStr_ << ": "
                     << ::testing::PrintToString(locked);
        } else {
          locked = std::nullopt;
          XLOG(INFO) << "Received delete for " << pathStr_;
        }
      });
    };
    if (!isStat_) {
      pubSubManager_->addStatePathSubscription(
          path_, streamStateChange, dataUpdate);
    } else {
      pubSubManager_->addStatPathSubscription(
          path_, streamStateChange, dataUpdate);
    }
  }

  ~TestSubscription() {
    if (!isStat_) {
      pubSubManager_->removeStatePathSubscription(path_);
    } else {
      pubSubManager_->removeStatPathSubscription(path_);
    }
  }

  folly::Synchronized<std::optional<typename Path::DataT>>& data() {
    return data_;
  }

 private:
  fsdb::FsdbPubSubManager* pubSubManager_;
  folly::Synchronized<std::optional<typename Path::DataT>> data_;
  std::vector<std::string> path_;
  std::string pathStr_;
  bool isStat_;
};

} // namespace facebook::fboss
