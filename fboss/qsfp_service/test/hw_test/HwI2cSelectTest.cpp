// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/test/hw_test/HwTest.h"

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"

#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.h"
#include "thrift/lib/cpp/util/EnumUtils.h"

#include <folly/gen/Base.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

/*
 * This test reads the serial numbers of all connected transceivers and verify
 * that they are unique. If they are not, the test will fail. Note an exception
 * is connected DACs because the transceivers from both ends will have the same
 * serial number.
 */
TEST_F(HwTest, i2cUniqueSerialNumbers) {
  // Get the IDs of all connected transceivers
  auto ensemble = getHwQsfpEnsemble();
  WedgeManager* wedgeManager = ensemble->getWedgeManager();
  auto agentConfig = wedgeManager->getAgentConfig();
  auto transceivers = utility::legacyTransceiverIds(
      utility::getCabledPortTranceivers(*agentConfig, ensemble));

  // Get information of all connected transceivers
  std::map<int32_t, TransceiverInfo> transceiverInfo;
  wedgeManager->getTransceiversInfo(
      transceiverInfo, std::make_unique<std::vector<int32_t>>(transceivers));

  // Build a dictionary of transceiver ID, its ethernet name (eg eth1/1/1), and
  // the ethernet name of the connected transceiver from the other end
  std::unordered_map<int32_t, std::pair<std::string, std::string>> cabledNames;
  auto& swConfig = *(agentConfig->thrift.sw());

  for (auto& port : *swConfig.ports()) {
    if (!port.expectedLLDPValues()->empty()) {
      auto portName = *port.Port::name();
      auto neighborName = port.expectedLLDPValues()->at(cfg::LLDPTag::PORT);
      auto portId = *port.logicalID();
      const int32_t id = static_cast<int32_t>(
          *utility::getTranscieverIdx(PortID(portId), ensemble));
      cabledNames[id] = {portName, neighborName};
      XLOG(INFO) << fmt::format(
          "Transceiver {:d} has name: {:s} and neighbor's name: {:s}",
          id,
          portName,
          neighborName);
    }
  }

  // Get the serial numbers of all connected transceivers and insert them into
  // a dictionary if they are unique
  std::unordered_map<std::string, int32_t> snIds;

  for (auto tcvrId : transceivers) {
    auto vendor = transceiverInfo[tcvrId].tcvrState()->vendor();
    CHECK(vendor);
    auto vendorInfo = *vendor;
    auto sn = *(vendorInfo.serialNumber());
    XLOG(INFO) << fmt::format(
        "Transceiver {:d} has serial number: {:s}", tcvrId, sn);

    if (snIds.find(sn) == snIds.end()) {
      snIds[sn] = tcvrId;
    } else {
      // Found duplicated serial numbers, module must be DAC and connected to
      // another DAC on the other end
      auto neighborId = snIds[sn];
      CHECK(cabledNames.find(tcvrId) != cabledNames.end());
      CHECK(cabledNames.find(neighborId) != cabledNames.end());
      EXPECT_EQ(cabledNames[tcvrId].first, cabledNames[neighborId].second);
      EXPECT_EQ(cabledNames[tcvrId].second, cabledNames[neighborId].first);

      auto transmitterTech =
          *(transceiverInfo[tcvrId].tcvrState()->cable()->transmitterTech());
      EXPECT_TRUE(TransmitterTechnology::COPPER == transmitterTech);

      transmitterTech = *(
          transceiverInfo[neighborId].tcvrState()->cable()->transmitterTech());
      EXPECT_TRUE(TransmitterTechnology::COPPER == transmitterTech);
    }
  }
}
} // namespace facebook::fboss
