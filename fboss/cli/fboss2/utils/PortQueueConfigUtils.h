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

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss::utils {

/**
 * A queue id plus the attributes to apply to it.
 *
 * Parses command line arguments in the format:
 *   <queue-id> [<attr1> <val1...> [<attr2> <val2...> ...]]
 *
 * Every attribute takes a single value token except rate-limit, which takes
 * two or three (<kbps|pps> [<min>] <max>).
 *
 * For example:
 *   0 reserved-bytes 1000 weight 10 scheduling WEIGHTED_ROUND_ROBIN
 *   0 rate-limit kbps 12500000 12500000
 */
class QueueIdAndAttributes : public BaseObjectArgType<std::string> {
 public:
  // NOLINTNEXTLINE(google-explicit-constructor)
  /* implicit */ QueueIdAndAttributes(std::vector<std::string> v);

  int16_t getQueueId() const {
    return queueId_;
  }

  const std::vector<std::pair<std::string, std::vector<std::string>>>&
  getAttributes() const {
    return attributes_;
  }

  const std::vector<std::string>& getAqmAttributes() const {
    return aqmAttributes_;
  }

 private:
  int16_t queueId_{0};
  std::vector<std::pair<std::string, std::vector<std::string>>> attributes_;
  std::vector<std::string> aqmAttributes_;
};

// Human-readable list of the supported queue attributes. Shared by the CLI help
// text and the parser's error messages so the two cannot drift.
const std::string& validQueueAttrs();

// Walks the `<attr> <value...>` stream starting at v[begin] into
// `attributes`, with `active-queue-management` (or `aqm`) consuming every
// remaining token into `aqmAttributes` -- and throwing when that tail is
// empty, so a trailing bare keyword cannot silently no-op. The one grammar
// both `config qos queue-config` and `config copp queue` parse, kept in one
// place so they cannot drift.
void walkQueueAttributes(
    const std::vector<std::string>& v,
    size_t begin,
    std::vector<std::pair<std::string, std::vector<std::string>>>& attributes,
    std::vector<std::string>& aqmAttributes);

// Applies queue configuration attributes onto `queue`. Shared by every command
// that configures cfg::PortQueue attributes -- the `config qos queue-config
// <name|default>` family and `config copp queue` -- which write the same
// struct into different parts of the switch config.
//
// `attributes` maps each attribute to its value tokens: one for the scalars
// (name, reserved-bytes, shared-bytes, max-dynamic-shared-bytes, weight,
// scaling-factor, scheduling, stream-type, buffer-pool-name) and two or three
// for rate-limit (<kbps|pps> [<min>] <max>). `aqmArgs` is the
// active-queue-management sub-arg stream.
//
// bandwidthBurstMinKbits/bandwidthBurstMaxKbits are deliberately absent: the
// SAI scheduler profile is created from a five-attribute tuple that has no
// burst-rate member (SaiSchedulerTraits::AdapterHostKey), so those two fields
// reach the running config and stop there. Exposing them would let an operator
// set a burst that never programs anything.
//
// Merges onto the given queue: fields not named are left untouched, and an AQM
// edit targets the entry matching its congestion-behavior (appending a new one
// so ECN and EARLY_DROP entries coexist rather than clobbering aqms[0]).
//
// Throws std::invalid_argument on an empty edit (both lists empty) or any
// invalid, unknown, or duplicated attribute; the caller is expected to apply
// the mutation transactionally (edit a copy, splice back only on success).
void applyPortQueueConfig(
    cfg::PortQueue& queue,
    const std::vector<std::pair<std::string, std::vector<std::string>>>&
        attributes,
    const std::vector<std::string>& aqmArgs);

} // namespace facebook::fboss::utils
