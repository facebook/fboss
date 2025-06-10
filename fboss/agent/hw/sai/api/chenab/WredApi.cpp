// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/api/WredApi.h"

#include "fboss/lib/TupleUtils.h"

/* TODO(Chenab) : Remove this once separate thresholds for ECN and early drop
 * are supported */

namespace {
using SaiWredTraits = facebook::fboss::SaiWredTraits;
using Attributes = facebook::fboss::SaiWredTraits::Attributes;
using ECNAttributes = std::tuple<
    Attributes::GreenEnable,
    Attributes::EcnMarkMode,
    std::optional<Attributes::EcnGreenMinThreshold>,
    std::optional<Attributes::EcnGreenMaxThreshold>>;
using EarlyDropAttributes = std::tuple<
    Attributes::GreenEnable,
    Attributes::EcnMarkMode,
    std::optional<Attributes::GreenMinThreshold>,
    std::optional<Attributes::GreenMaxThreshold>,
    std::optional<Attributes::GreenDropProbability>>;

std::variant<ECNAttributes, EarlyDropAttributes> separateAttributes(
    const SaiWredTraits::AdapterHostKey& key) {
  auto mode = std::get<Attributes::EcnMarkMode>(key);
  if (mode == SAI_ECN_MARK_MODE_GREEN) {
    return tupleProjection<SaiWredTraits::AdapterHostKey, ECNAttributes>(key);
  }
  if (mode == SAI_ECN_MARK_MODE_NONE) {
    return tupleProjection<SaiWredTraits::AdapterHostKey, EarlyDropAttributes>(
        key);
  }
  XLOG(FATAL) << "Invalid ECN Mark Mode";
}
} // namespace

bool operator==(
    const SaiWredTraits::AdapterHostKey& left,
    const SaiWredTraits::AdapterHostKey& right) {
  return separateAttributes(left) == separateAttributes(right);
}

namespace std {
size_t hash<SaiWredTraits::AdapterHostKey>::operator()(
    const SaiWredTraits::AdapterHostKey& key) const {
  size_t seed = 0;
  auto attrs = separateAttributes(key);
  std::visit(
      [&seed](const auto& tup) {
        tupleForEach(
            [&seed](const auto& attr) {
              boost::hash_combine(
                  seed, std::hash<std::decay_t<decltype(attr)>>{}(attr));
            },
            tup);
      },
      attrs);
  return seed;
}

} // namespace std
