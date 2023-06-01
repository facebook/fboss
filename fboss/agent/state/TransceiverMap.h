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

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/state/Transceiver.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;

using TransceiverMapLegacyTraits =
    NodeMapTraits<TransceiverID, TransceiverSpec>;

using TransceiverMapTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::integral,
    apache::thrift::type_class::structure>;
using TransceiverMapThriftType =
    std::map<int16_t, state::TransceiverSpecFields>;

class TransceiverMap;
using TransceiverMapTraits = ThriftMapNodeTraits<
    TransceiverMap,
    TransceiverMapTypeClass,
    TransceiverMapThriftType,
    TransceiverSpec>;

/*
 * A container for all the present Transceivers
 */
class TransceiverMap
    : public ThriftMapNode<TransceiverMap, TransceiverMapTraits> {
 public:
  using Base = ThriftMapNode<TransceiverMap, TransceiverMapTraits>;
  using Traits = TransceiverMapTraits;
  using Base::modify;
  using LegacyTraits = TransceiverMapLegacyTraits;
  TransceiverMap();
  virtual ~TransceiverMap() override;

  const std::shared_ptr<TransceiverSpec> getTransceiver(
      TransceiverID id) const {
    return getNode(static_cast<int16_t>(id));
  }
  std::shared_ptr<TransceiverSpec> getTransceiverIf(TransceiverID id) const {
    return getNodeIf(static_cast<int16_t>(id));
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MultiSwitchTransceiverMapTypeClass = apache::thrift::type_class::
    map<apache::thrift::type_class::string, TransceiverMapTypeClass>;
using MultiSwitchTransceiverMapThriftType =
    std::map<std::string, TransceiverMapThriftType>;

class MultiSwitchTransceiverMap;

using MultiSwitchTransceiverMapTraits = ThriftMultiSwitchMapNodeTraits<
    MultiSwitchTransceiverMap,
    MultiSwitchTransceiverMapTypeClass,
    MultiSwitchTransceiverMapThriftType,
    TransceiverMap>;

class HwSwitchMatcher;

class MultiSwitchTransceiverMap : public ThriftMultiSwitchMapNode<
                                      MultiSwitchTransceiverMap,
                                      MultiSwitchTransceiverMapTraits> {
 public:
  using Traits = MultiSwitchTransceiverMapTraits;
  using BaseT = ThriftMultiSwitchMapNode<
      MultiSwitchTransceiverMap,
      MultiSwitchTransceiverMapTraits>;
  using BaseT::modify;

  MultiSwitchTransceiverMap() {}
  virtual ~MultiSwitchTransceiverMap() {}

  MultiSwitchTransceiverMap* modify(std::shared_ptr<SwitchState>* state);

 private:
  // Inherit the constructors required for clone()
  using BaseT::BaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
