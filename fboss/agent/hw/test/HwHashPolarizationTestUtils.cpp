// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwHashPolarizationTestUtils.h"

#include "fboss/agent/packet/PktUtil.h"

#include <folly/io/Cursor.h>

namespace facebook::fboss {

namespace {
std::unique_ptr<std::vector<utility::EthFrame>> getEthFrames(
    const std::vector<std::string>& packets) {
  std::unique_ptr<std::vector<utility::EthFrame>> frames =
      std::make_unique<std::vector<utility::EthFrame>>();
  for (auto pktString : packets) {
    auto ioBuf = PktUtil::parseHexData(pktString);
    auto cursor = folly::io::Cursor(&ioBuf);
    frames->emplace_back(cursor);
  }
  return frames;
}
} // namespace

std::unique_ptr<std::vector<utility::EthFrame>> getFullHashedPackets(
    HwAsic::AsicType asic,
    bool isSai) {
  if (isSai) {
    switch (asic) {
      case HwAsic::AsicType::ASIC_TYPE_TRIDENT2:
        return getEthFrames(getFullHashedPacketsForTrident2Sai());
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
        return getEthFrames(getFullHashedPacketsForTomahawkSai());

      case HwAsic::AsicType::ASIC_TYPE_FAKE:
      case HwAsic::AsicType::ASIC_TYPE_MOCK:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
      case HwAsic::AsicType::ASIC_TYPE_TAJO:
      case HwAsic::AsicType::ASIC_TYPE_ELBERT_8DD:
        return nullptr;
    }
  } else {
    switch (asic) {
      case HwAsic::AsicType::ASIC_TYPE_TRIDENT2:
        return getEthFrames(getFullHashedPacketsForTrident2());
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
        return getEthFrames(getFullHashedPacketsForTomahawk());
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
        return getEthFrames(getFullHashedPacketsForTomahawk3());
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
        return getEthFrames(getFullHashedPacketsForTomahawk4());

      case HwAsic::AsicType::ASIC_TYPE_FAKE:
      case HwAsic::AsicType::ASIC_TYPE_MOCK:
      case HwAsic::AsicType::ASIC_TYPE_TAJO:
      case HwAsic::AsicType::ASIC_TYPE_ELBERT_8DD:
        return nullptr;
    }
  }
}

} // namespace facebook::fboss
