/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiHashManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

namespace {
using namespace facebook::fboss;

SaiHashTraits::Attributes::NativeHashFieldList toNativeHashFieldList(
    const cfg::Fields& hashFields) {
  SaiHashTraits::Attributes::NativeHashFieldList::ValueType nativeHashFields;
  for (auto field : *hashFields.ipv4Fields_ref()) {
    switch (field) {
      case cfg::IPv4Field::SOURCE_ADDRESS:
        nativeHashFields.emplace_back(SAI_NATIVE_HASH_FIELD_SRC_IP);
        break;
      case cfg::IPv4Field::DESTINATION_ADDRESS:
        nativeHashFields.emplace_back(SAI_NATIVE_HASH_FIELD_DST_IP);
        break;
    }
  }
  for (auto field : *hashFields.ipv6Fields_ref()) {
    switch (field) {
      case cfg::IPv6Field::SOURCE_ADDRESS:
        nativeHashFields.emplace_back(SAI_NATIVE_HASH_FIELD_SRC_IP);
        break;
      case cfg::IPv6Field::DESTINATION_ADDRESS:
        nativeHashFields.emplace_back(SAI_NATIVE_HASH_FIELD_DST_IP);
        break;
      case cfg::IPv6Field::FLOW_LABEL:
        throw FbossError("Hashing on Flow labels is not supported");
    }
  }
  for (auto field : *hashFields.transportFields_ref()) {
    switch (field) {
      case cfg::TransportField::SOURCE_PORT:
        nativeHashFields.emplace_back(SAI_NATIVE_HASH_FIELD_L4_SRC_PORT);
        break;
      case cfg::TransportField::DESTINATION_PORT:
        nativeHashFields.emplace_back(SAI_NATIVE_HASH_FIELD_L4_DST_PORT);
        break;
    }
  }
  if (hashFields.mplsFields_ref()->size()) {
    throw FbossError("Hashing on MPLS fields is not supported");
  }
  return nativeHashFields;
}
} // namespace

namespace facebook::fboss {

std::shared_ptr<SaiHash> SaiHashManager::getOrCreate(
    const cfg::Fields& hashFields) {
  auto nativeHashFields = toNativeHashFieldList(hashFields);
  SaiHashTraits::AdapterHostKey adapterHostKey{nativeHashFields, std::nullopt};
  SaiHashTraits::CreateAttributes createAttrs{nativeHashFields, std::nullopt};

  return handles_
      .refOrEmplace(
          adapterHostKey,
          adapterHostKey,
          createAttrs,
          managerTable_->switchManager().getSwitchSaiId())
      .first;
}
} // namespace facebook::fboss
