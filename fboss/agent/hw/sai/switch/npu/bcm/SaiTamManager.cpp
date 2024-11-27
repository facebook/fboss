// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiTamManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
extern "C" {
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiexperimentaltameventaginggroup.h>
#include <experimental/saitamextensions.h>
#else
#include <saiexperimentaltameventaginggroup.h>
#include <saitamextensions.h>
#endif
}

namespace {
// Must be less than BRCM_SAI_DNX_MOD_MAX_SUPP_AGE_TIME (776 on J3).
constexpr int kDefaultAgingIntervalUsecs = 500;

// TODO(maxgg): split into multiple profiles once CS00012378634 is fixed.
// Using ID 6 for now, for better visibility in packet dumps.
const std::map<int, std::vector<sai_int32_t>> kDropProfiles = {
    {6,
     {
         // 0 - Global resources
         SAI_PACKET_DROP_TYPE_MMU_GLOBAL_DRAM_BDBS,
         SAI_PACKET_DROP_TYPE_MMU_GLOBAL_SRAM_BUFFERS,
         SAI_PACKET_DROP_TYPE_MMU_GLOBAL_SRAM_PDBS,

         // 1 - VOQ resources
         SAI_PACKET_DROP_TYPE_MMU_VOQ_SRAM_PDS_TOTAL_FREE_SHARED,
         SAI_PACKET_DROP_TYPE_MMU_VOQ_SRAM_PDS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VOQ_SRAM_BUFFERS_TOTAL_FREE_SHARED,
         SAI_PACKET_DROP_TYPE_MMU_VOQ_SRAM_BUFFERS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VOQ_WORDS_TOTAL_FREE_SHARED,
         SAI_PACKET_DROP_TYPE_MMU_VOQ_WORDS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VOQ_DRAM_BLOCK,

         // 2 - VSQ resources
         SAI_PACKET_DROP_TYPE_MMU_PB_VSQ_SRAM_PDS_TOTAL_FREE_SHARED,
         SAI_PACKET_DROP_TYPE_MMU_PB_VSQ_SRAM_PDS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_PB_VSQ_SRAM_BUFFERS_TOTAL_FREE_SHARED,
         SAI_PACKET_DROP_TYPE_MMU_PB_VSQ_SRAM_BUFFERS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_PB_VSQ_WORDS_TOTAL_FREE_SHARED,
         SAI_PACKET_DROP_TYPE_MMU_PB_VSQ_WORDS_SHARED_MAX_SIZE,

         // 3 - VSQ D resources
         SAI_PACKET_DROP_TYPE_MMU_VSQ_D_SRAM_PDS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_D_SRAM_BUFFERS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_D_WORDS_SHARED_MAX_SIZE,

         // 4 - Queue and packet processing
         SAI_PACKET_DROP_TYPE_MMU_QUEUE_NUM_RESOLUTION_ERROR,
         SAI_PACKET_DROP_TYPE_MMU_QUEUE_NUM_NOT_VALID,
         SAI_PACKET_DROP_TYPE_MMU_PP_ERROR,

         // 5 - Misc
         SAI_PACKET_DROP_TYPE_MMU_ITPP_DELTA_ERROR,
         SAI_PACKET_DROP_TYPE_MMU_DROP_PRECEDENCE_LEVEL,
         SAI_PACKET_DROP_TYPE_MMU_SRAM_RESOURCE_ERROR,
         SAI_PACKET_DROP_TYPE_MMU_EXTERNAL_ERROR,
         SAI_PACKET_DROP_TYPE_MMU_MACSEC_ERROR,
         SAI_PACKET_DROP_TYPE_MMU_TAR_FIFO_FULL,
         SAI_PACKET_DROP_TYPE_MMU_PACKET_SIZE_ERROR,

         // 6 - Unexpected reasons
         SAI_PACKET_DROP_TYPE_MMU_LAG_REMOTE,
         SAI_PACKET_DROP_TYPE_MMU_LAG_PROTECTION,
         SAI_PACKET_DROP_TYPE_MMU_LATENCY,
         SAI_PACKET_DROP_TYPE_MMU_MULTICAST_REPLICATION_ERROR,
         SAI_PACKET_DROP_TYPE_MMU_MULTICAST_FIFO_FULL,
         SAI_PACKET_DROP_TYPE_MMU_VOQ_SYSTEM_RED,
         SAI_PACKET_DROP_TYPE_MMU_VOQ_WRED,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_F_WRED,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_E_WRED,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_C_SRAM_PDS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_B_SRAM_PDS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_A_SRAM_PDS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_C_SRAM_BUFFERS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_B_SRAM_BUFFERS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_A_SRAM_BUFFERS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_C_WORDS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_B_WORDS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_A_WORDS_SHARED_MAX_SIZE,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_D_WRED,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_C_WRED,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_B_WRED,
         SAI_PACKET_DROP_TYPE_MMU_VSQ_A_WRED,
     }},
};
} // namespace
#endif

namespace facebook::fboss {

SaiTamManager::SaiTamManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

void SaiTamManager::addMirrorOnDropReport(
    const std::shared_ptr<MirrorOnDropReport>& report) {
#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
  // Create report
  auto& reportStore = saiStore_->get<SaiTamReportTraits>();
  auto reportTraits =
      SaiTamReportTraits::CreateAttributes{SAI_TAM_REPORT_TYPE_MOD_OVER_UDP};
  auto reportObj = reportStore.setObject(reportTraits, reportTraits);

  // Create action
  auto& actionStore = saiStore_->get<SaiTamEventActionTraits>();
  auto actionTraits =
      SaiTamEventActionTraits::CreateAttributes{reportObj->adapterKey()};
  auto action = actionStore.setObject(actionTraits, actionTraits);

  // Create transport
  auto& transportStore = saiStore_->get<SaiTamTransportTraits>();
  auto transportTraits = SaiTamTransportTraits::AdapterHostKey{
      SAI_TAM_TRANSPORT_TYPE_PORT,
      report->getLocalSrcPort(),
      report->getCollectorPort(),
      report->getMtu(),
      folly::MacAddress(report->getSwitchMac()),
      folly::MacAddress(report->getFirstInterfaceMac()),
  };
  auto transport = transportStore.setObject(transportTraits, transportTraits);

  // Create collector
  auto& collectorStore = saiStore_->get<SaiTamCollectorTraits>();
  auto collectorTraits = SaiTamCollectorTraits::CreateAttributes{
      report->getLocalSrcIp(),
      report->getCollectorIp(),
      report->getTruncateSize(),
      transport->adapterKey(),
      report->getDscp(),
  };
  auto collector = collectorStore.setObject(collectorTraits, collectorTraits);

  std::vector<std::shared_ptr<SaiTamEventAgingGroup>> agingGroups;
  std::vector<std::shared_ptr<SaiTamEvent>> events;
  std::vector<sai_object_id_t> eventIds;
  for (const auto& [eventId, reasons] : kDropProfiles) {
    // Create aging group
    auto& agingGroupStore = saiStore_->get<SaiTamEventAgingGroupTraits>();
    sai_uint16_t agingInterval =
        report->getAgingIntervalUsecs().value_or(kDefaultAgingIntervalUsecs) +
        eventId;
    auto agingGroupTraits = SaiTamEventAgingGroupTraits::CreateAttributes{
        SAI_TAM_EVENT_AGING_GROUP_TYPE_VOQ, agingInterval};
    auto agingGroup =
        agingGroupStore.setObject(agingGroupTraits, agingGroupTraits);
    agingGroups.push_back(agingGroup);

    // Create event
    std::vector<sai_object_id_t> actions{action->adapterKey()};
    std::vector<sai_object_id_t> collectors{collector->adapterKey()};
    sai_object_id_t agingGroupKey = agingGroup->adapterKey();
    SaiTamEventTraits::CreateAttributes eventTraits;
    std::get<SaiTamEventTraits::Attributes::Type>(eventTraits) =
        SAI_TAM_EVENT_TYPE_PACKET_DROP_STATEFUL;
    std::get<SaiTamEventTraits::Attributes::ActionList>(eventTraits) = actions;
    std::get<std::optional<SaiTamEventTraits::Attributes::DeviceId>>(
        eventTraits) = 0; // specify 0 explcitily so that round trip works
    std::get<std::optional<SaiTamEventTraits::Attributes::SwitchEventId>>(
        eventTraits) = eventId;
    std::get<
        std::optional<SaiTamEventTraits::Attributes::ExtensionsCollectorList>>(
        eventTraits) = collectors;
    std::get<std::optional<SaiTamEventTraits::Attributes::PacketDropTypeMmu>>(
        eventTraits) = reasons;
    std::get<std::optional<SaiTamEventTraits::Attributes::AgingGroup>>(
        eventTraits) = agingGroupKey;
    auto& eventStore = saiStore_->get<SaiTamEventTraits>();
    auto event = eventStore.setObject(eventTraits, eventTraits);
    events.push_back(event);
    eventIds.push_back(event->adapterKey());
    +XLOG(INFO) << "Created event ID " << eventId << " with tam event "
                << event->adapterKey() << " and aging group " << agingGroupKey;
  }

  // Create tam
  std::vector<sai_int32_t> bindpoints = {
      SAI_TAM_BIND_POINT_TYPE_SWITCH, SAI_TAM_BIND_POINT_TYPE_PORT};
  SaiTamTraits::CreateAttributes tamTraits;
  std::get<SaiTamTraits::Attributes::EventObjectList>(tamTraits) = eventIds;
  std::get<SaiTamTraits::Attributes::TamBindPointList>(tamTraits) = bindpoints;
  auto& tamStore = saiStore_->get<SaiTamTraits>();
  auto tam = tamStore.setObject(tamTraits, tamTraits);

  // Associate TAM with port
  XLOG(INFO) << "Associating TAM object with port "
             << report->getMirrorPortId();
  managerTable_->portManager().setTamObject(
      PortID(report->getMirrorPortId()), {tam->adapterKey()});

  // Associate TAM with switch
  managerTable_->switchManager().setTamObject({tam->adapterKey()});

  auto tamHandle = std::make_unique<SaiTamHandle>();
  tamHandle->report = reportObj;
  tamHandle->action = action;
  tamHandle->transport = transport;
  tamHandle->collector = collector;
  tamHandle->agingGroups = agingGroups;
  tamHandle->events = events;
  tamHandle->tam = tam;
  tamHandle->portId = report->getMirrorPortId();
  tamHandles_.emplace(report->getID(), std::move(tamHandle));
#endif
}

void SaiTamManager::removeMirrorOnDropReport(
    const std::shared_ptr<MirrorOnDropReport>& report) {
  tamHandles_.erase(report->getID());
}

void SaiTamManager::changeMirrorOnDropReport(
    const std::shared_ptr<MirrorOnDropReport>& oldReport,
    const std::shared_ptr<MirrorOnDropReport>& newReport) {
  removeMirrorOnDropReport(oldReport);
  addMirrorOnDropReport(newReport);
}

std::vector<PortID> SaiTamManager::getAllMirrorOnDropPortIds() {
  std::vector<PortID> portIds;
  for (const auto& [_, tamHandle] : tamHandles_) {
    portIds.push_back(tamHandle->portId);
  }
  return portIds;
}

} // namespace facebook::fboss
