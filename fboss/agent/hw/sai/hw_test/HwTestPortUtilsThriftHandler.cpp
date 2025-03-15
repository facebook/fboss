// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/diag/DiagShell.h"
#include "fboss/agent/hw/sai/switch/SaiLagManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"
#include "fboss/agent/platforms/common/utils/Wedge100LedUtils.h"

#include "folly/testing/TestUtil.h"

namespace facebook {
namespace fboss {
namespace utility {

namespace {
SaiPortTraits::AdapterKey getPortAdapterKey(const HwSwitch* hw, PortID port) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto handle = saiSwitch->managerTable()->portManager().getPortHandle(port);
  CHECK(handle);
  return handle->port->adapterKey();
}
} // namespace

void HwTestThriftHandler::injectFecError(
    std::unique_ptr<std::vector<int>> hwPorts,
    bool injectCorrectable) {
  if (hwSwitch_->getPlatform()->getAsic()->getAsicVendor() !=
      HwAsic::AsicVendor::ASIC_VENDOR_BCM) {
    throw FbossError("FEC error injection only supported on BCM ASICs");
  }
  auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch_);
  auto diagShell = std::make_unique<DiagShell>(saiSwitch);
  auto diagCmdServer =
      std::make_unique<DiagCmdServer>(saiSwitch, diagShell.get());

  constexpr auto kBaseFecErrorInjStr = R"(
  cint_reset();
  bcm_port_phy_fec_error_mask_t injection;
  uint8 error_control_map = 0xFF;
  int unit = 0;
  )";
  std::string kFecErrorInjStr = kBaseFecErrorInjStr;
  if (injectCorrectable) {
    kFecErrorInjStr = folly::sformat(
        "\n{}\n injection.error_mask_bit_63_32=0x1;", kFecErrorInjStr);
  } else {
    kFecErrorInjStr = folly::sformat(
        "\n{}\n injection.error_mask_bit_63_32=0xFFFFFFFF;", kFecErrorInjStr);
  }
  for (auto hwPortID : *hwPorts) {
    int portNum = hwPortID;
    kFecErrorInjStr = folly::sformat(
        "\n{}\n print bcm_port_phy_fec_error_inject_set(unit, {}, error_control_map, injection);",
        kFecErrorInjStr,
        portNum);
  }
  folly::test::TemporaryFile file;
  folly::writeFull(
      file.fd(), kFecErrorInjStr.c_str(), std::strlen(kFecErrorInjStr.c_str()));

  XLOG(INFO) << "CINT = " << kFecErrorInjStr;
  ClientInformation clientInfo;
  clientInfo.username() = "hw_test";
  clientInfo.hostname() = "hw_test";
  auto out = diagCmdServer->diagCmd(
      std::make_unique<fbstring>(
          folly::sformat("cint {}\n", file.path().c_str())),
      std::make_unique<ClientInformation>(clientInfo));
  XLOG(INFO) << "OUTPUT = " << out;
  diagCmdServer->diagCmd(
      std::make_unique<fbstring>("quit\n"),
      std::make_unique<ClientInformation>(clientInfo));
}

void HwTestThriftHandler::getPortInfo(
    ::std::vector<::facebook::fboss::utility::PortInfo>& portInfos,
    std::unique_ptr<::std::vector<::std::int32_t>> portIds) {
  for (const auto& portId : *portIds) {
    PortInfo portInfo;
    auto key = getPortAdapterKey(hwSwitch_, PortID(portId));
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
    SaiPortTraits::Attributes::PortLoopbackMode loopbackMode;
    SaiApiTable::getInstance()->portApi().getAttribute(key, loopbackMode);
    portInfo.loopbackMode() = loopbackMode.value();
#else
    SaiPortTraits::Attributes::InternalLoopbackMode internalLoopbackMode;
    SaiApiTable::getInstance()->portApi().getAttribute(
        key, internalLoopbackMode);
    portInfo.loopbackMode() = internalLoopbackMode.value();
#endif
    portInfos.push_back(portInfo);
  }
  return;
}

bool HwTestThriftHandler::verifyPortLedStatus(int portId, bool status) {
  SaiPlatform* platform = static_cast<SaiPlatform*>(hwSwitch_->getPlatform());
  SaiPlatformPort* platformPort = platform->getPort(PortID(portId));
  uint32_t currentVal = platformPort->getCurrentLedState();
  uint32_t expectedVal = 0;
  switch (platform->getType()) {
    case PlatformType::PLATFORM_WEDGE100: {
      expectedVal = static_cast<uint32_t>(Wedge100LedUtils::getExpectedLEDState(
          platform->getLaneCount(platformPort->getCurrentProfile()),
          status,
          status));
      return currentVal == expectedVal;
    }
    default:
      throw FbossError("Unsupported platform type");
  }
}

bool HwTestThriftHandler::verifyPGSettings(int portId, bool pfcEnabled) {
  auto swPort = hwSwitch_->getProgrammedState()->getPorts()->getNodeIf(portId);
  auto swPgConfig = swPort->getPortPgConfigs();

  auto portHandle = static_cast<const SaiSwitch*>(hwSwitch_)
                        ->managerTable()
                        ->portManager()
                        .getPortHandle(PortID(swPort->getID()));
  // Ensure that both SW and HW has the same number of PG IDs
  if (portHandle->configuredIngressPriorityGroups.size() !=
      swPort->getPortPgConfigs()->size()) {
    XLOG(DBG2) << "Number of PGs mismatch for port " << swPort->getName()
               << " hw size: "
               << portHandle->configuredIngressPriorityGroups.size()
               << " sw size: " << swPort->getPortPgConfigs()->size();
    return false;
  }
  for (const auto& pgConfig : std::as_const(*swPgConfig)) {
    auto id = pgConfig->cref<switch_state_tags::id>()->cref();
    auto iter = portHandle->configuredIngressPriorityGroups.find(
        static_cast<IngressPriorityGroupID>(id));
    if (iter == portHandle->configuredIngressPriorityGroups.end()) {
      XLOG(DBG2) << "Priority group config canot be found for PG id " << id
                 << " on port " << swPort->getName();
      return false;
    }
    auto bufferProfile = iter->second.bufferProfile;
    if (pgConfig->cref<switch_state_tags::resumeOffsetBytes>()->cref() !=
        SaiApiTable::getInstance()->bufferApi().getAttribute(
            bufferProfile->adapterKey(),
            SaiBufferProfileTraits::Attributes::XonOffsetTh{})) {
      XLOG(DBG2) << "Resume offset mismatch for pg " << id;
      return false;
    }
    if (pgConfig->cref<switch_state_tags::minLimitBytes>()->cref() !=
        SaiApiTable::getInstance()->bufferApi().getAttribute(
            bufferProfile->adapterKey(),
            SaiBufferProfileTraits::Attributes::ReservedBytes{})) {
      XLOG(DBG2) << "Min limit mismatch for pg " << id;
      return false;
    }
    if (pgConfig->cref<switch_state_tags::minLimitBytes>()->cref() !=
        SaiApiTable::getInstance()->bufferApi().getAttribute(
            bufferProfile->adapterKey(),
            SaiBufferProfileTraits::Attributes::ReservedBytes{})) {
      XLOG(DBG2) << "Min limit mismatch for pg " << id;
      return false;
    }
    if (auto pgHdrmOpt =
            pgConfig->cref<switch_state_tags::headroomLimitBytes>()) {
      if (pgHdrmOpt->cref() !=
          SaiApiTable::getInstance()->bufferApi().getAttribute(
              bufferProfile->adapterKey(),
              SaiBufferProfileTraits::Attributes::XoffTh{})) {
        XLOG(DBG2) << "Headroom mismatch for pg " << id;
        return false;
      }
    }

    // Buffer pool configs
    const auto bufferPool =
        pgConfig->cref<switch_state_tags::bufferPoolConfig>();
    if (bufferPool->cref<common_if_tags::headroomBytes>()->cref() *
            static_cast<const SaiSwitch*>(hwSwitch_)
                ->getPlatform()
                ->getAsic()
                ->getNumMemoryBuffers() !=
        SaiApiTable::getInstance()->bufferApi().getAttribute(
            static_cast<const SaiSwitch*>(hwSwitch_)
                ->managerTable()
                ->bufferManager()
                .getIngressBufferPoolHandle()
                ->bufferPool->adapterKey(),
            SaiBufferPoolTraits::Attributes::XoffSize{})) {
      XLOG(DBG2) << "Headroom mismatch for buffer pool";
      return false;
    }

    // Port PFC configurations
    if (SaiApiTable::getInstance()->portApi().getAttribute(
            portHandle->port->adapterKey(),
            SaiPortTraits::Attributes::PriorityFlowControlMode{}) ==
        SAI_PORT_PRIORITY_FLOW_CONTROL_MODE_COMBINED) {
      auto hwPfcEnabled = SaiApiTable::getInstance()->portApi().getAttribute(
                              portHandle->port->adapterKey(),
                              SaiPortTraits::Attributes::PriorityFlowControl{})
          ? 1
          : 0;
      if (hwPfcEnabled != pfcEnabled) {
        XLOG(DBG2) << "PFC mismatch for port " << swPort->getName();
        return false;
      }
    } else {
#if !defined(TAJO_SDK)
      auto hwPfcEnabled =
          SaiApiTable::getInstance()->portApi().getAttribute(
              portHandle->port->adapterKey(),
              SaiPortTraits::Attributes::PriorityFlowControlTx{})
          ? 1
          : 0;
      if (hwPfcEnabled != pfcEnabled) {
        XLOG(DBG2) << "PFC mismatch for port " << swPort->getName();
        return false;
      }
#else
      XLOG(DBG2) << "Flow control mode SEPARATE unsupported!";
      return false;
#endif
    }
  }
  return true;
}

void HwTestThriftHandler::getAggPortInfo(
    ::std::vector<::facebook::fboss::utility::AggPortInfo>& aggPortInfos,
    std::unique_ptr<::std::vector<::std::int32_t>> aggPortIds) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch_);
  auto& lagManager = saiSwitch->managerTable()->lagManager();
  for (const auto& portId : *aggPortIds) {
    AggPortInfo aggPortInfo;
    AggregatePortID aggPortId = AggregatePortID(portId);
    try {
      lagManager.getLagHandle(aggPortId);
      aggPortInfo.isPresent() = true;
      aggPortInfo.numMembers() = lagManager.getLagMemberCount(aggPortId);
      aggPortInfo.numActiveMembers() =
          lagManager.getActiveMemberCount(aggPortId);

    } catch (const std::exception& e) {
      XLOG(DBG2) << "Lag handle not found for port " << aggPortId;
      aggPortInfo.isPresent() = false;
    }
    aggPortInfos.push_back(aggPortInfo);
  }
  return;
}

int HwTestThriftHandler::getNumAggPorts() {
  auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch_);
  return saiSwitch->managerTable()->lagManager().getLagCount();
}

bool HwTestThriftHandler::verifyPktFromAggPort(int aggPortId) {
  std::array<char, 8> data{};
  // TODO (T159867926): Set the right queue ID once the vendor
  // set the right queue ID in the rx callback.
  auto rxPacket = std::make_unique<SaiRxPacket>(
      data.size(),
      data.data(),
      AggregatePortID(aggPortId),
      VlanID(1),
      cfg::PacketRxReason::UNMATCHED,
      0 /* queue Id */);
  return rxPacket->isFromAggregatePort();
}

} // namespace utility
} // namespace fboss
} // namespace facebook
