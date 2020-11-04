// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

class TamStoreTest : public SaiStoreTest {
 public:
  void SetUp() override {
    SaiStoreTest::SetUp();
  }

  SaiTamReportTraits::CreateAttributes tamReportTraits() {
    return SaiTamReportTraits::CreateAttributes{
        SAI_TAM_REPORT_TYPE_VENDOR_EXTN};
  }

  SaiTamEventActionTraits::CreateAttributes tamEventActionTraits(
      TamReportSaiId id) {
    return SaiTamEventActionTraits::CreateAttributes{id};
  }

  SaiTamEventTraits::CreateAttributes tamEventTraits(
      TamEventActionSaiId eventActionId) {
    std::vector<sai_object_id_t> actions{eventActionId};
    std::vector<sai_object_id_t> collectors{SAI_NULL_OBJECT_ID};
    std::vector<sai_int32_t> eventTypes{1, 2, 3, 4};

    SaiTamEventTraits::CreateAttributes result;
    std::get<SaiTamEventTraits::Attributes::Type>(result) =
        SAI_TAM_EVENT_TYPE_PACKET_DROP;
    std::get<SaiTamEventTraits::Attributes::ActionList>(result) = actions;
    std::get<SaiTamEventTraits::Attributes::CollectorList>(result) = collectors;
    std::get<SaiTamEventTraits::Attributes::SwitchEventType>(result) =
        eventTypes;

    return result;
  }

  SaiTamTraits::CreateAttributes tamTraits(TamEventSaiId event) {
    std::vector<sai_object_id_t> events = {event};
    std::vector<sai_int32_t> bindpoints = {SAI_TAM_BIND_POINT_TYPE_SWITCH};
    return SaiTamTraits::CreateAttributes{events, bindpoints};
  }

  TamReportSaiId createReport() {
    return saiApiTable->tamApi().create<SaiTamReportTraits>(
        tamReportTraits(), 0);
  }

  TamEventActionSaiId createEventAction(TamReportSaiId reportId) {
    return saiApiTable->tamApi().create<SaiTamEventActionTraits>(
        tamEventActionTraits(reportId), 0);
  }

  TamEventSaiId createEvent(TamEventActionSaiId actionId) {
    return saiApiTable->tamApi().create<SaiTamEventTraits>(
        tamEventTraits(actionId), 0);
  }

  TamSaiId createTam(TamEventSaiId eventId) {
    return saiApiTable->tamApi().create<SaiTamTraits>(tamTraits(eventId), 0);
  }
};

TEST_F(TamStoreTest, loadTam) {
  auto report = createReport();
  auto action = createEventAction(report);
  auto event = createEvent(action);
  auto tam = createTam(event);

  SaiStore s(0);
  s.reload();

  auto& reportStore = s.get<SaiTamReportTraits>();
  auto& actionStore = s.get<SaiTamEventActionTraits>();
  auto& eventStore = s.get<SaiTamEventTraits>();
  auto& tamStore = s.get<SaiTamTraits>();
  EXPECT_EQ(
      reportStore
          .get(SaiTamReportTraits::AdapterHostKey{
              SAI_TAM_REPORT_TYPE_VENDOR_EXTN})
          ->adapterKey(),
      report);
  EXPECT_EQ(
      actionStore.get(SaiTamEventActionTraits::AdapterHostKey{report})
          ->adapterKey(),
      action);
  EXPECT_EQ(eventStore.get(tamEventTraits(action))->adapterKey(), event);
  EXPECT_EQ(tamStore.get(tamTraits(event))->adapterKey(), tam);
}

TEST_F(TamStoreTest, tamCtors) {
  auto report = createReport();
  auto action = createEventAction(report);
  auto event = createEvent(action);
  auto tam = createTam(event);

  auto reportObj = createObj<SaiTamReportTraits>(report);
  EXPECT_EQ(
      GET_ATTR(TamReport, Type, reportObj.attributes()),
      SAI_TAM_REPORT_TYPE_VENDOR_EXTN);

  auto actionObj = createObj<SaiTamEventActionTraits>(action);
  EXPECT_EQ(
      GET_ATTR(TamEventAction, ReportType, actionObj.attributes()), report);

  auto eventObj = createObj<SaiTamEventTraits>(event);
  auto tamEventAhk = tamEventTraits(action);
  EXPECT_EQ(
      GET_ATTR(TamEvent, Type, eventObj.attributes()),
      SAI_TAM_EVENT_TYPE_PACKET_DROP);
  EXPECT_EQ(
      GET_ATTR(TamEvent, ActionList, eventObj.attributes()),
      std::get<SaiTamEventTraits::Attributes::ActionList>(tamEventAhk).value());
  EXPECT_EQ(
      GET_ATTR(TamEvent, CollectorList, eventObj.attributes()),
      std::get<SaiTamEventTraits::Attributes::CollectorList>(tamEventAhk)
          .value());
  EXPECT_EQ(
      GET_ATTR(TamEvent, SwitchEventType, eventObj.attributes()),
      std::get<SaiTamEventTraits::Attributes::SwitchEventType>(tamEventAhk)
          .value());

  auto tamObj = createObj<SaiTamTraits>(tam);
  auto tamAhk = tamTraits(event);
  EXPECT_EQ(
      GET_ATTR(Tam, EventObjectList, tamObj.attributes()),
      std::get<SaiTamTraits::Attributes::EventObjectList>(tamAhk).value());
  EXPECT_EQ(
      GET_ATTR(Tam, TamBindPointList, tamObj.attributes()),
      std::get<SaiTamTraits::Attributes::TamBindPointList>(tamAhk).value());
}

TEST_F(TamStoreTest, setObject) {
  SaiStore s(0);
  s.reload();
  auto reportKey = tamReportTraits();
  auto report = s.get<SaiTamReportTraits>().setObject(reportKey, reportKey);
  EXPECT_EQ(
      GET_ATTR(TamReport, Type, report->attributes()),
      SAI_TAM_REPORT_TYPE_VENDOR_EXTN);

  auto actionAhk = tamEventActionTraits(report->adapterKey());
  auto action =
      s.get<SaiTamEventActionTraits>().setObject(actionAhk, actionAhk);
  EXPECT_EQ(
      GET_ATTR(TamEventAction, ReportType, action->attributes()),
      report->adapterKey());

  auto eventAhk = tamEventTraits(action->adapterKey());
  auto event = s.get<SaiTamEventTraits>().setObject(eventAhk, eventAhk);

  EXPECT_EQ(
      GET_ATTR(TamEvent, Type, event->attributes()),
      SAI_TAM_EVENT_TYPE_PACKET_DROP);
  EXPECT_EQ(
      GET_ATTR(TamEvent, ActionList, event->attributes()),
      std::get<SaiTamEventTraits::Attributes::ActionList>(eventAhk).value());
  EXPECT_EQ(
      GET_ATTR(TamEvent, CollectorList, event->attributes()),
      std::get<SaiTamEventTraits::Attributes::CollectorList>(eventAhk).value());
  EXPECT_EQ(
      GET_ATTR(TamEvent, SwitchEventType, event->attributes()),
      std::get<SaiTamEventTraits::Attributes::SwitchEventType>(eventAhk)
          .value());

  auto tamAhk = tamTraits(event->adapterKey());
  auto tam = s.get<SaiTamTraits>().setObject(tamAhk, tamAhk);
  EXPECT_EQ(
      GET_ATTR(Tam, EventObjectList, tam->attributes()),
      std::get<SaiTamTraits::Attributes::EventObjectList>(tamAhk).value());
  EXPECT_EQ(
      GET_ATTR(Tam, TamBindPointList, tam->attributes()),
      std::get<SaiTamTraits::Attributes::TamBindPointList>(tamAhk).value());
}

TEST_F(TamStoreTest, updateObject) {
  SaiStore s(0);
  s.reload();
  auto reportKey = tamReportTraits();
  auto report = s.get<SaiTamReportTraits>().setObject(reportKey, reportKey);

  auto actionAhk = tamEventActionTraits(report->adapterKey());
  auto action =
      s.get<SaiTamEventActionTraits>().setObject(actionAhk, actionAhk);

  auto eventAhk = tamEventTraits(action->adapterKey());
  auto event = s.get<SaiTamEventTraits>().setObject(eventAhk, eventAhk);

  auto tamAhk = tamTraits(event->adapterKey());
  auto tam = s.get<SaiTamTraits>().setObject(tamAhk, tamAhk);

  std::vector<sai_int32_t> newEvents = {4, 5, 6};
  std::get<SaiTamEventTraits::Attributes::SwitchEventType>(eventAhk) =
      newEvents;
  auto updatedEvent = s.get<SaiTamEventTraits>().setObject(eventAhk, eventAhk);
  EXPECT_EQ(
      GET_ATTR(TamEvent, SwitchEventType, updatedEvent->attributes()),
      newEvents);
}
