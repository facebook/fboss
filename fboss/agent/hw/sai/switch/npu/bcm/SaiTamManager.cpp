// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiTamManager.h"

#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

extern "C" {
#if !defined(SAI_VERSION_5_0_0_7_ODP)
#include <experimental/saiexperimentalswitch.h>
#include <experimental/saitamextensions.h>
#endif
}
namespace facebook::fboss {

SaiTamHandle::~SaiTamHandle() {
  managerTable->switchManager().resetTamObject();
}

SaiTamManager::SaiTamManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore),
      managerTable_(managerTable),
      platform_(platform),
      tamHandle_(std::make_unique<SaiTamHandle>()) {
#if !defined(SAI_VERSION_5_0_0_7_ODP)
  // create report
  auto& reportStore = saiStore_->get<SaiTamReportTraits>();
  auto reportTraits =
      SaiTamReportTraits::CreateAttributes{SAI_TAM_REPORT_TYPE_VENDOR_EXTN};
  auto report = reportStore.setObject(reportTraits, reportTraits);

  // create action
  auto& actionStore = saiStore_->get<SaiTamEventActionTraits>();
  auto actionTraits =
      SaiTamEventActionTraits::CreateAttributes{report->adapterKey()};
  auto action = actionStore.setObject(actionTraits, actionTraits);

  // create event
  SaiTamEventTraits::CreateAttributes eventTraits;
  std::vector<sai_object_id_t> actions{action->adapterKey()};
  std::vector<sai_object_id_t> collectors{};
  std::vector<sai_int32_t> eventTypes{
      SAI_SWITCH_EVENT_TYPE_STABLE_FULL,
      SAI_SWITCH_EVENT_TYPE_STABLE_ERROR,
      SAI_SWITCH_EVENT_TYPE_UNCONTROLLED_SHUTDOWN,
      SAI_SWITCH_EVENT_TYPE_WARM_BOOT_DOWNGRADE,
      SAI_SWITCH_EVENT_TYPE_PARITY_ERROR};
  std::get<SaiTamEventTraits::Attributes::Type>(eventTraits) =
      SAI_TAM_EVENT_TYPE_SWITCH;
  std::get<SaiTamEventTraits::Attributes::ActionList>(eventTraits) = actions;
  std::get<SaiTamEventTraits::Attributes::CollectorList>(eventTraits) =
      collectors;
  std::get<SaiTamEventTraits::Attributes::SwitchEventType>(eventTraits) =
      eventTypes;
  auto& eventStore = saiStore_->get<SaiTamEventTraits>();
  auto event = eventStore.setObject(eventTraits, eventTraits);

  // create tam
  std::vector<sai_object_id_t> events = {event->adapterKey()};
  std::vector<sai_int32_t> bindpoints = {SAI_TAM_BIND_POINT_TYPE_SWITCH};
  SaiTamTraits::CreateAttributes tamTraits;
  std::get<SaiTamTraits::Attributes::EventObjectList>(tamTraits) = events;
  std::get<SaiTamTraits::Attributes::TamBindPointList>(tamTraits) = bindpoints;
  auto& tamStore = saiStore_->get<SaiTamTraits>();
  auto tam = tamStore.setObject(tamTraits, tamTraits);

  tamHandle_->report = report;
  tamHandle_->action = action;
  tamHandle_->event = event;
  tamHandle_->tam = tam;
  tamHandle_->managerTable = managerTable_;
  // associate TAM with switch
  managerTable_->switchManager().setTamObject({tamHandle_->tam->adapterKey()});
#endif
}

} // namespace facebook::fboss
