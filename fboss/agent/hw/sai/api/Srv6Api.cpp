// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/api/Srv6Api.h"

#include "fboss/agent/Constants.h"

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

namespace std {

size_t hash<facebook::fboss::SaiMySidEntryTraits::MySidEntry>::operator()(
    const facebook::fboss::SaiMySidEntryTraits::MySidEntry& entry) const {
  size_t seed = 0;
  boost::hash_combine(seed, entry.switchId());
  boost::hash_combine(seed, entry.vrId());
  boost::hash_combine(seed, entry.locatorBlockLen());
  boost::hash_combine(seed, entry.locatorNodeLen());
  boost::hash_combine(seed, entry.functionLen());
  boost::hash_combine(seed, entry.argsLen());
  boost::hash_combine(seed, std::hash<folly::IPAddressV6>()(entry.sid()));
  return seed;
}

} // namespace std

namespace facebook::fboss {

std::string SaiMySidEntryTraits::MySidEntry::toString() const {
  return folly::to<std::string>(
      "MySidEntry(switch: ",
      switchId(),
      ", vr: ",
      vrId(),
      ", locatorBlockLen: ",
      locatorBlockLen(),
      ", locatorNodeLen: ",
      locatorNodeLen(),
      ", functionLen: ",
      functionLen(),
      ", argsLen: ",
      argsLen(),
      ", sid: ",
      sid(),
      ")");
}

constexpr folly::StringPiece kVrId{"vr_id"};
constexpr folly::StringPiece kLocatorBlockLen{"locator_block_len"};
constexpr folly::StringPiece kLocatorNodeLen{"locator_node_len"};
constexpr folly::StringPiece kFunctionLen{"function_len"};
constexpr folly::StringPiece kArgsLen{"args_len"};
constexpr folly::StringPiece kSid{"sid"};

folly::dynamic SaiMySidEntryTraits::MySidEntry::toFollyDynamic() const {
  folly::dynamic json = folly::dynamic::object;
  json[kSwitchId] = switchId();
  json[kVrId] = vrId();
  json[kLocatorBlockLen] = locatorBlockLen();
  json[kLocatorNodeLen] = locatorNodeLen();
  json[kFunctionLen] = functionLen();
  json[kArgsLen] = argsLen();
  json[kSid] = sid().str();
  return json;
}

SaiMySidEntryTraits::MySidEntry
SaiMySidEntryTraits::MySidEntry::fromFollyDynamic(const folly::dynamic& json) {
  sai_object_id_t switchId = json[kSwitchId].asInt();
  sai_object_id_t vrId = json[kVrId].asInt();
  uint8_t locatorBlockLen = json[kLocatorBlockLen].asInt();
  uint8_t locatorNodeLen = json[kLocatorNodeLen].asInt();
  uint8_t functionLen = json[kFunctionLen].asInt();
  uint8_t argsLen = json[kArgsLen].asInt();
  folly::IPAddressV6 sid(json[kSid].asString());
  return MySidEntry(
      switchId,
      vrId,
      locatorBlockLen,
      locatorNodeLen,
      functionLen,
      argsLen,
      sid);
}

} // namespace facebook::fboss

#endif
