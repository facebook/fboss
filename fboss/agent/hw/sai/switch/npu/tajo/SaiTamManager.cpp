// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiTamManager.h"

#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

extern "C" {
#include <experimental/sai_attr_ext.h>
}

namespace facebook::fboss {

SaiTamManager::SaiTamManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::TELEMETRY_AND_MONITORING)) {
    return;
  }

  // To address SMS out of bank error seen in S337271, TAJO has a new
  // notification which will notify FBOSS agent on SMS out of bank
  // errors in the fleet via SAI_SWITCH_EVENT_TYPE_LACK_OF_RESOURCES.
  // However, this would need the SwitchEventType to be modified and
  // TAJO has a limitation that only one TAM object can be created at
  // any time. This means, warm boot upgrade is not possible to a new
  // version which will try to create a new TAM object as that would
  // end up with 2 TAM objects being created and the create would fail.
  // The work around is to delete the TAM object and creating a new one.
  if (platform_->getHwSwitch()->getBootType() == BootType::WARM_BOOT) {
    // Reset the tam object to start with.
    // TODO: As of now, resetTamObject() API fails for TAJO, while this
    // is being fixed in MT-665, using the work around to setTamObject()
    // with empty vector to reset.
    managerTable_->switchManager().setTamObject({});
    saiStore_->get<SaiTamTraits>().removeUnexpectedUnclaimedWarmbootHandles();
    saiStore_->get<SaiTamEventTraits>()
        .removeUnexpectedUnclaimedWarmbootHandles();
    saiStore_->get<SaiTamEventActionTraits>()
        .removeUnexpectedUnclaimedWarmbootHandles();
    saiStore_->get<SaiTamReportTraits>()
        .removeUnexpectedUnclaimedWarmbootHandles();
  }

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
  std::vector<sai_int32_t> eventTypes{SAI_SWITCH_EVENT_TYPE_PARITY_ERROR};
  std::get<SaiTamEventTraits::Attributes::Type>(eventTraits) =
      SAI_TAM_EVENT_TYPE_SWITCH;
  std::get<SaiTamEventTraits::Attributes::ActionList>(eventTraits) = actions;
  std::get<SaiTamEventTraits::Attributes::CollectorList>(eventTraits) =
      collectors;
  std::get<std::optional<SaiTamEventTraits::Attributes::SwitchEventType>>(
      eventTraits) = eventTypes;
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

  auto tamHandle = std::make_unique<SaiTamHandle>();
  tamHandle->report = report;
  tamHandle->action = action;
  tamHandle->events.push_back(event);
  tamHandle->tam = tam;
  tamHandle->managerTable = managerTable_;
  // associate TAM with switch
  managerTable_->switchManager().setTamObject({tamHandle->tam->adapterKey()});

  tamHandles_.emplace("default", std::move(tamHandle));
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
