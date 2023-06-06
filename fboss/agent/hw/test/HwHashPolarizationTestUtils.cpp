// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwHashPolarizationTestUtils.h"

#include "fboss/agent/packet/PktUtil.h"

#include <folly/io/Cursor.h>
#include <folly/logging/xlog.h>

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

std::vector<std::string> toStringVec(const std::vector<std::string_view>& in) {
  std::vector<std::string> ret(in.begin(), in.end());

  return ret;
}

std::unique_ptr<std::vector<utility::EthFrame>> getFullHashedPackets(
    cfg::AsicType asic,
    bool isSai) {
  if (isSai) {
    switch (asic) {
      case cfg::AsicType::ASIC_TYPE_TRIDENT2:
        return getEthFrames(getFullHashedPacketsForTrident2Sai());
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
        return getEthFrames(getFullHashedPacketsForTomahawkSai());

      case cfg::AsicType::ASIC_TYPE_FAKE:
      case cfg::AsicType::ASIC_TYPE_MOCK:
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      case cfg::AsicType::ASIC_TYPE_EBRO:
      case cfg::AsicType::ASIC_TYPE_GARONNE:
      case cfg::AsicType::ASIC_TYPE_YUBA:
      case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
      case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
      case cfg::AsicType::ASIC_TYPE_JERICHO2:
      case cfg::AsicType::ASIC_TYPE_JERICHO3:
      case cfg::AsicType::ASIC_TYPE_RAMON:
        return nullptr;
    }
  } else {
    switch (asic) {
      case cfg::AsicType::ASIC_TYPE_TRIDENT2:
        return getEthFrames(getFullHashedPacketsForTrident2());
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
        return getEthFrames(getFullHashedPacketsForTomahawk());
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
        return getEthFrames(getFullHashedPacketsForTomahawk3());
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
        return getEthFrames(getFullHashedPacketsForTomahawk4());

      case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      case cfg::AsicType::ASIC_TYPE_FAKE:
      case cfg::AsicType::ASIC_TYPE_MOCK:
      case cfg::AsicType::ASIC_TYPE_EBRO:
      case cfg::AsicType::ASIC_TYPE_GARONNE:
      case cfg::AsicType::ASIC_TYPE_YUBA:
      case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
      case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
      case cfg::AsicType::ASIC_TYPE_JERICHO2:
      case cfg::AsicType::ASIC_TYPE_JERICHO3:
      case cfg::AsicType::ASIC_TYPE_RAMON:
        return nullptr;
    }
  }
  XLOG(FATAL) << " Should never get here";
  return nullptr;
}

} // namespace facebook::fboss
