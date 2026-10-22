// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "fboss/agent/hw/sai/switch/SaiTamManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/lib/TupleUtils.h"

namespace facebook::fboss {

SaiTamManager::SaiTamManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

std::shared_ptr<SaiTamReport> SaiTamManager::createTamReport(
    sai_int32_t reportType) {
  SaiTamReportTraits::CreateAttributes attributes{
      reportType
#if SAI_API_VERSION >= SAI_VERSION(1, 16, 0)
      ,
      std::nullopt, // SampleRate
      std::nullopt, // MaxReportRate
      std::nullopt // MaxReportBurst
#endif
  };
  return saiStore_->get<SaiTamReportTraits>().setObject(attributes, attributes);
}

std::shared_ptr<SaiTamEventAction> SaiTamManager::createTamAction(
    sai_object_id_t reportId) {
  SaiTamEventActionTraits::CreateAttributes attributes{reportId};
  return saiStore_->get<SaiTamEventActionTraits>().setObject(
      attributes, attributes);
}

std::shared_ptr<SaiTamTransport> SaiTamManager::createTamTransport() {
  SaiTamTransportTraits::CreateAttributes attributes{
      SAI_TAM_TRANSPORT_TYPE_GRE,
      std::nullopt, // SrcPort
      std::nullopt, // DstPort
      std::nullopt, // Mtu
      std::nullopt, // SrcMacAddress
      std::nullopt, // DstMacAddress
      sai_uint16_t{0x6558},
  };
  return saiStore_->get<SaiTamTransportTraits>().setObject(
      attributes, attributes);
}

std::shared_ptr<SaiTamCollector> SaiTamManager::createTamCollector(
    const std::shared_ptr<MirrorOnDropReport>& report,
    sai_object_id_t transportId,
    sai_object_id_t destinationId) {
  const auto resolvedCollectorMac = report->getResolvedCollectorMac();
  CHECK(resolvedCollectorMac.has_value());
  SaiTamCollectorTraits::CreateAttributes attributes{
      report->getLocalSrcIp(),
      report->getCollectorIp(),
      static_cast<sai_uint16_t>(report->getTruncateSize()),
      transportId,
      report->getDscp(),
      folly::MacAddress{report->getFirstInterfaceMac()},
      *resolvedCollectorMac,
      destinationId,
      false,
  };
  return saiStore_->get<SaiTamCollectorTraits>().setObject(
      attributes, attributes);
}

std::shared_ptr<SaiTamEvent> SaiTamManager::createTamEvent(
    sai_object_id_t actionId,
    sai_object_id_t collectorId) {
  SaiTamEventTraits::CreateAttributes attributes;
  std::get<SaiTamEventTraits::Attributes::Type>(attributes) =
      SAI_TAM_EVENT_TYPE_PACKET_DROP;
  std::get<SaiTamEventTraits::Attributes::ActionList>(attributes) =
      std::vector<sai_object_id_t>{actionId};
  std::get<SaiTamEventTraits::Attributes::CollectorList>(attributes) =
      std::vector<sai_object_id_t>{collectorId};
  auto adapterHostKey = tupleProjection<
      SaiTamEventTraits::CreateAttributes,
      SaiTamEventTraits::AdapterHostKey>(attributes);
  return saiStore_->get<SaiTamEventTraits>().setObject(
      adapterHostKey, attributes);
}

std::shared_ptr<SaiTam> SaiTamManager::createTam(
    const std::vector<sai_object_id_t>& eventIds,
    const std::vector<sai_int32_t>& bindpoints) {
  SaiTamTraits::CreateAttributes attributes{eventIds, bindpoints};
  return saiStore_->get<SaiTamTraits>().setObject(attributes, attributes);
}

void SaiTamManager::addMirrorOnDropReport(
    const std::shared_ptr<MirrorOnDropReport>& /* report */) {}

void SaiTamManager::removeMirrorOnDropReport(
    const std::shared_ptr<MirrorOnDropReport>& /* report */) {}

void SaiTamManager::changeMirrorOnDropReport(
    const std::shared_ptr<MirrorOnDropReport>& /* oldReport */,
    const std::shared_ptr<MirrorOnDropReport>& /* newReport */) {}

std::vector<PortID> SaiTamManager::getAllMirrorOnDropPortIds() {
  return {};
}

} // namespace facebook::fboss
