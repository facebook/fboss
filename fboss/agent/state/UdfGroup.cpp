/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/UdfGroup.h"
#include "fboss/agent/gen-cpp2/switch_config_fatal.h"
#include "fboss/agent/gen-cpp2/switch_config_fatal_types.h"

namespace facebook::fboss {

UdfGroup::UdfGroup(const std::string& name)
    : ThriftStructNode<UdfGroup, cfg::UdfGroup>() {
  set<switch_config_tags::name>(name);
}

std::string UdfGroup::getName() const {
  return get<switch_config_tags::name>()->cref();
}

std::optional<cfg::UdfGroupType> UdfGroup::getUdfGroupType() const {
  if (auto udfGroupType = cref<switch_config_tags::type>()) {
    return udfGroupType->cref();
  }
  return std::nullopt;
}

// THRIFT_COPY: avoid returning std::vector
std::vector<std::string> UdfGroup::getUdfPacketMatcherIds() const {
  return get<switch_config_tags::udfPacketMatcherIds>()->toThrift();
}

cfg::UdfBaseHeaderType UdfGroup::getUdfBaseHeader() const {
  return get<switch_config_tags::header>()->cref();
}

int UdfGroup::getStartOffsetInBytes() const {
  return get<switch_config_tags::startOffsetInBytes>()->cref();
}

int UdfGroup::getFieldSizeInBytes() const {
  return get<switch_config_tags::fieldSizeInBytes>()->cref();
}

void UdfGroup::setUdfGroupType(std::optional<cfg::UdfGroupType> type) {
  if (type) {
    set<switch_config_tags::type>(*type);
  } else {
    ref<switch_config_tags::type>().reset();
  }
}
void UdfGroup::setUdfBaseHeader(cfg::UdfBaseHeaderType header) {
  set<switch_config_tags::header>(header);
}
void UdfGroup::setStartOffsetInBytes(int offset) {
  set<switch_config_tags::startOffsetInBytes>(offset);
}
void UdfGroup::setFieldSizeInBytes(int size) {
  set<switch_config_tags::fieldSizeInBytes>(size);
}
void UdfGroup::setUdfPacketMatcherIds(
    const std::vector<std::string>& matcherIds) {
  set<switch_config_tags::udfPacketMatcherIds>(matcherIds);
}

template struct ThriftStructNode<UdfGroup, cfg::UdfGroup>;
} // namespace facebook::fboss
