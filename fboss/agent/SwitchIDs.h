// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <algorithm>
#include <initializer_list>

#include <boost/container/static_vector.hpp>
#include <folly/logging/xlog.h>

#include "fboss/agent/types.h"

namespace facebook::fboss {

/*
 * Maximum number of local switch IDs (NPUs) that SwitchIDs can hold.
 * Sized to cover foreseeable multi-NPU platforms. static_vector throws
 * on overflow; the runtime CHECK in MultiHwSwitchHandler also fires on
 * startup if a platform exceeds this.
 */
inline constexpr size_t kMaxSwitchIds = 32;

/*
 * A stack-allocated, sorted, deduplicated collection of SwitchIDs for
 * use in performance-sensitive packet TX paths.
 *
 * Backed by boost::container::static_vector with capacity for up to
 * kMaxSwitchIds elements, eliminating heap allocation for all
 * foreseeable NPU counts. Call-site syntax is unchanged:
 *
 *   sendPacketSwitchedAsync(std::move(pkt), {switchId});
 *   sendPacketSwitchedAsync(std::move(pkt), {id1, id2});
 *   sendPacketSwitchedAsync(std::move(pkt));
 *
 * Duplicate SwitchIDs are detected, logged as errors, and
 * deduplicated — packets are sent exactly once per unique SwitchID.
 */
class SwitchIDs {
 public:
  using container_type =
      boost::container::static_vector<SwitchID, kMaxSwitchIds>;
  using const_iterator = container_type::const_iterator;

  SwitchIDs() = default;

  /* implicit */ SwitchIDs(std::initializer_list<SwitchID> ids)
      : ids_(ids.begin(), ids.end()) {
    std::sort(ids_.begin(), ids_.end());
    auto dupIt = std::adjacent_find(ids_.begin(), ids_.end());
    if (dupIt != ids_.end()) {
      XLOG(ERR) << "Duplicate SwitchID " << *dupIt << " in packet send targets";
    }
    ids_.erase(std::unique(ids_.begin(), ids_.end()), ids_.end());
  }

  bool empty() const {
    return ids_.empty();
  }
  size_t size() const {
    return ids_.size();
  }
  const_iterator begin() const {
    return ids_.begin();
  }
  const_iterator end() const {
    return ids_.end();
  }

 private:
  container_type ids_;
};

} // namespace facebook::fboss
