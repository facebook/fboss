// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/MacAddress.h>

#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

using namespace facebook::fboss;

class TamStoreTest : public SaiStoreTest {
 public:
  void SetUp() override {
    SaiStoreTest::SetUp();
  }

  const folly::IPAddress kTamCollectorSrcIpV4{"1.1.1.1"};
  const folly::IPAddress kTamCollectorDstIpV4{"2.2.2.2"};
  const folly::IPAddress kTamCollectorSrcIpV6{"2401::1"};
  const folly::IPAddress kTamCollectorDstIpV6{"2401::2"};

  facebook::fboss::SaiTamCollectorTraits::CreateAttributes tamCollectorTraits(
      facebook::fboss::TamTransportSaiId transportId,
      bool ipV4) {
    SaiTamCollectorTraits::CreateAttributes result;
    std::get<SaiTamCollectorTraits::Attributes::SrcIp>(result) =
        ipV4 ? kTamCollectorSrcIpV4 : kTamCollectorSrcIpV6;
    std::get<SaiTamCollectorTraits::Attributes::DstIp>(result) =
        ipV4 ? kTamCollectorDstIpV4 : kTamCollectorDstIpV6;
    std::get<std::optional<SaiTamCollectorTraits::Attributes::TruncateSize>>(
        result) = 128;
    std::get<SaiTamCollectorTraits::Attributes::Transport>(result) =
        SaiTamCollectorTraits::Attributes::Transport{transportId};
    std::get<std::optional<SaiTamCollectorTraits::Attributes::DscpValue>>(
        result) = 0;
    return result;
  }

  facebook::fboss::SaiTamTransportTraits::AdapterHostKey tamTransportTraits() {
    SaiTamTransportTraits::AdapterHostKey result;
    std::get<SaiTamTransportTraits::Attributes::Type>(result) =
        SAI_TAM_TRANSPORT_TYPE_UDP;
    std::get<SaiTamTransportTraits::Attributes::SrcPort>(result) = 10001;
    std::get<SaiTamTransportTraits::Attributes::DstPort>(result) = 10002;
    std::get<SaiTamTransportTraits::Attributes::Mtu>(result) = 1500;
    std::get<std::optional<SaiTamTransportTraits::Attributes::SrcMacAddress>>(
        result) = folly::MacAddress("00:00:00:00:00:01");
    std::get<std::optional<SaiTamTransportTraits::Attributes::DstMacAddress>>(
        result) = folly::MacAddress("00:00:00:00:00:02");
    return result;
  }

  facebook::fboss::SaiTamReportTraits::CreateAttributes tamReportTraits() {
    return SaiTamReportTraits::CreateAttributes{
        SAI_TAM_REPORT_TYPE_VENDOR_EXTN};
  }

  facebook::fboss::SaiTamEventActionTraits::CreateAttributes
  tamEventActionTraits(facebook::fboss::TamReportSaiId id) {
    return SaiTamEventActionTraits::CreateAttributes{id};
  }

  facebook::fboss::SaiTamEventTraits::CreateAttributes tamEventTraits(
      facebook::fboss::TamEventActionSaiId eventActionId) {
    std::vector<sai_object_id_t> actions{eventActionId};
    std::vector<sai_object_id_t> collectors{SAI_NULL_OBJECT_ID};
    std::vector<sai_int32_t> eventTypes{1, 2, 3, 4};

    sai_int32_t deviceId = 0;
    sai_int32_t eventId = 1;
    std::vector<sai_object_id_t> extensionsCollectorList{10};
    std::vector<sai_int32_t> packetDropTypeMmu = {3, 4};
    sai_object_id_t agingGroup = 20;

    SaiTamEventTraits::CreateAttributes result;
    std::get<SaiTamEventTraits::Attributes::Type>(result) =
        SAI_TAM_EVENT_TYPE_PACKET_DROP;
    std::get<SaiTamEventTraits::Attributes::ActionList>(result) = actions;
    std::get<SaiTamEventTraits::Attributes::CollectorList>(result) = collectors;
    std::get<std::optional<SaiTamEventTraits::Attributes::SwitchEventType>>(
        result) = eventTypes;
    std::get<std::optional<SaiTamEventTraits::Attributes::DeviceId>>(result) =
        deviceId;
    std::get<std::optional<SaiTamEventTraits::Attributes::SwitchEventId>>(
        result) = eventId;
    std::get<
        std::optional<SaiTamEventTraits::Attributes::ExtensionsCollectorList>>(
        result) = extensionsCollectorList;
    std::get<std::optional<SaiTamEventTraits::Attributes::PacketDropTypeMmu>>(
        result) = packetDropTypeMmu;
    std::get<std::optional<SaiTamEventTraits::Attributes::AgingGroup>>(result) =
        agingGroup;

    return result;
  }

  facebook::fboss::SaiTamTraits::CreateAttributes tamTraits(
      facebook::fboss::TamEventSaiId event) {
    std::vector<sai_object_id_t> events = {event};
    std::vector<sai_int32_t> bindpoints = {SAI_TAM_BIND_POINT_TYPE_SWITCH};
    return SaiTamTraits::CreateAttributes{events, bindpoints};
  }

  facebook::fboss::TamCollectorSaiId createCollector(
      facebook::fboss::TamTransportSaiId transportId,
      bool ipV4) {
    return saiApiTable->tamApi().create<SaiTamCollectorTraits>(
        tamCollectorTraits(transportId, ipV4), ipV4 ? 0 : 1);
  }

  facebook::fboss::TamTransportSaiId createTransport() {
    return saiApiTable->tamApi().create<SaiTamTransportTraits>(
        tamTransportTraits(), 0);
  }

  facebook::fboss::TamReportSaiId createReport() {
    return saiApiTable->tamApi().create<SaiTamReportTraits>(
        tamReportTraits(), 0);
  }

  facebook::fboss::TamEventActionSaiId createEventAction(
      facebook::fboss::TamReportSaiId reportId) {
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
  auto transport = createTransport();
  auto collectorV4 = createCollector(transport, true /* ipV4 */);
  auto collectorV6 = createCollector(transport, false /* ipV4 */);
  auto report = createReport();
  auto action = createEventAction(report);
  auto event = createEvent(action);
  auto tam = createTam(event);

  SaiStore s(0);
  s.reload();

  auto& collectorStore = s.get<SaiTamCollectorTraits>();
  auto& transportStore = s.get<SaiTamTransportTraits>();
  auto& reportStore = s.get<SaiTamReportTraits>();
  auto& actionStore = s.get<SaiTamEventActionTraits>();
  auto& eventStore = s.get<SaiTamEventTraits>();
  auto& tamStore = s.get<SaiTamTraits>();
  EXPECT_EQ(
      collectorStore.get(tamCollectorTraits(transport, true /* ipV4 */))
          ->adapterKey(),
      collectorV4);
  EXPECT_EQ(
      collectorStore.get(tamCollectorTraits(transport, false /* ipV4 */))
          ->adapterKey(),
      collectorV6);
  EXPECT_EQ(transportStore.get(tamTransportTraits())->adapterKey(), transport);
  EXPECT_EQ(
      reportStore
          .get(
              SaiTamReportTraits::AdapterHostKey{
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
  auto transport = createTransport();
  auto collectorV4 = createCollector(transport, true /* ipV4 */);
  auto collectorV6 = createCollector(transport, false /* ipV4 */);
  auto report = createReport();
  auto action = createEventAction(report);
  auto event = createEvent(action);
  auto tam = createTam(event);

  auto transportObj = createObj<SaiTamTransportTraits>(transport);
  auto tamTransportAhk = tamTransportTraits();
  EXPECT_EQ(
      GET_ATTR(TamTransport, Type, transportObj.attributes()),
      std::get<SaiTamTransportTraits::Attributes::Type>(tamTransportAhk)
          .value());
  EXPECT_EQ(
      GET_ATTR(TamTransport, SrcPort, transportObj.attributes()),
      std::get<SaiTamTransportTraits::Attributes::SrcPort>(tamTransportAhk)
          .value());
  EXPECT_EQ(
      GET_ATTR(TamTransport, DstPort, transportObj.attributes()),
      std::get<SaiTamTransportTraits::Attributes::DstPort>(tamTransportAhk)
          .value());
  EXPECT_EQ(
      GET_ATTR(TamTransport, Mtu, transportObj.attributes()),
      std::get<SaiTamTransportTraits::Attributes::Mtu>(tamTransportAhk)
          .value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamTransport, SrcMacAddress, transportObj.attributes()),
      std::get<std::optional<SaiTamTransportTraits::Attributes::SrcMacAddress>>(
          tamTransportAhk)
          .value()
          .value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamTransport, DstMacAddress, transportObj.attributes()),
      std::get<std::optional<SaiTamTransportTraits::Attributes::DstMacAddress>>(
          tamTransportAhk)
          .value()
          .value());

  auto collectorObjV4 = createObj<SaiTamCollectorTraits>(collectorV4);
  auto tamCollectorAhkV4 = tamCollectorTraits(transport, true /* ipV4 */);
  EXPECT_EQ(
      GET_ATTR(TamCollector, SrcIp, collectorObjV4.attributes()),
      std::get<SaiTamCollectorTraits::Attributes::SrcIp>(tamCollectorAhkV4)
          .value());
  EXPECT_EQ(
      GET_ATTR(TamCollector, DstIp, collectorObjV4.attributes()),
      std::get<SaiTamCollectorTraits::Attributes::DstIp>(tamCollectorAhkV4)
          .value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamCollector, TruncateSize, collectorObjV4.attributes()),
      std::get<std::optional<SaiTamCollectorTraits::Attributes::TruncateSize>>(
          tamCollectorAhkV4)
          .value()
          .value());
  EXPECT_EQ(
      GET_ATTR(TamCollector, Transport, collectorObjV4.attributes()),
      std::get<SaiTamCollectorTraits::Attributes::Transport>(tamCollectorAhkV4)
          .value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamCollector, DscpValue, collectorObjV4.attributes()),
      std::get<std::optional<SaiTamCollectorTraits::Attributes::DscpValue>>(
          tamCollectorAhkV4)
          .value()
          .value());

  auto collectorObjV6 = createObj<SaiTamCollectorTraits>(collectorV6);
  auto tamCollectorAhkV6 = tamCollectorTraits(transport, false /* ipV6 */);
  EXPECT_EQ(
      GET_ATTR(TamCollector, SrcIp, collectorObjV6.attributes()),
      std::get<SaiTamCollectorTraits::Attributes::SrcIp>(tamCollectorAhkV6)
          .value());
  EXPECT_EQ(
      GET_ATTR(TamCollector, DstIp, collectorObjV6.attributes()),
      std::get<SaiTamCollectorTraits::Attributes::DstIp>(tamCollectorAhkV6)
          .value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamCollector, TruncateSize, collectorObjV6.attributes()),
      std::get<std::optional<SaiTamCollectorTraits::Attributes::TruncateSize>>(
          tamCollectorAhkV6)
          .value()
          .value());
  EXPECT_EQ(
      GET_ATTR(TamCollector, Transport, collectorObjV6.attributes()),
      std::get<SaiTamCollectorTraits::Attributes::Transport>(tamCollectorAhkV6)
          .value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamCollector, DscpValue, collectorObjV6.attributes()),
      std::get<std::optional<SaiTamCollectorTraits::Attributes::DscpValue>>(
          tamCollectorAhkV6)
          .value()
          .value());

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
      GET_OPT_ATTR(TamEvent, SwitchEventType, eventObj.attributes()),
      std::get<std::optional<SaiTamEventTraits::Attributes::SwitchEventType>>(
          tamEventAhk)
          .value()
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

  auto transportAhk = tamTransportTraits();
  auto transport =
      s.get<SaiTamTransportTraits>().setObject(transportAhk, transportAhk);
  EXPECT_EQ(
      GET_ATTR(TamTransport, Type, transport->attributes()),
      std::get<SaiTamTransportTraits::Attributes::Type>(transportAhk).value());
  EXPECT_EQ(
      GET_ATTR(TamTransport, SrcPort, transport->attributes()),
      std::get<SaiTamTransportTraits::Attributes::SrcPort>(transportAhk)
          .value());
  EXPECT_EQ(
      GET_ATTR(TamTransport, DstPort, transport->attributes()),
      std::get<SaiTamTransportTraits::Attributes::DstPort>(transportAhk)
          .value());
  EXPECT_EQ(
      GET_ATTR(TamTransport, Mtu, transport->attributes()),
      std::get<SaiTamTransportTraits::Attributes::Mtu>(transportAhk).value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamTransport, SrcMacAddress, transport->attributes()),
      std::get<std::optional<SaiTamTransportTraits::Attributes::SrcMacAddress>>(
          transportAhk)
          .value()
          .value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamTransport, DstMacAddress, transport->attributes()),
      std::get<std::optional<SaiTamTransportTraits::Attributes::DstMacAddress>>(
          transportAhk)
          .value()
          .value());

  auto collectorAhkV4 =
      tamCollectorTraits(transport->adapterKey(), true /* ipV4 */);
  auto collectorV4 =
      s.get<SaiTamCollectorTraits>().setObject(collectorAhkV4, collectorAhkV4);
  EXPECT_EQ(
      GET_ATTR(TamCollector, SrcIp, collectorV4->attributes()),
      std::get<SaiTamCollectorTraits::Attributes::SrcIp>(collectorAhkV4)
          .value());
  EXPECT_EQ(
      GET_ATTR(TamCollector, DstIp, collectorV4->attributes()),
      std::get<SaiTamCollectorTraits::Attributes::DstIp>(collectorAhkV4)
          .value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamCollector, TruncateSize, collectorV4->attributes()),
      std::get<std::optional<SaiTamCollectorTraits::Attributes::TruncateSize>>(
          collectorAhkV4)
          .value()
          .value());
  EXPECT_EQ(
      GET_ATTR(TamCollector, Transport, collectorV4->attributes()),
      std::get<SaiTamCollectorTraits::Attributes::Transport>(collectorAhkV4)
          .value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamCollector, DscpValue, collectorV4->attributes()),
      std::get<std::optional<SaiTamCollectorTraits::Attributes::DscpValue>>(
          collectorAhkV4)
          .value()
          .value());

  auto collectorAhkV6 =
      tamCollectorTraits(transport->adapterKey(), false /* ipV6 */);
  auto collectorV6 =
      s.get<SaiTamCollectorTraits>().setObject(collectorAhkV6, collectorAhkV6);
  EXPECT_EQ(
      GET_ATTR(TamCollector, SrcIp, collectorV6->attributes()),
      std::get<SaiTamCollectorTraits::Attributes::SrcIp>(collectorAhkV6)
          .value());
  EXPECT_EQ(
      GET_ATTR(TamCollector, DstIp, collectorV6->attributes()),
      std::get<SaiTamCollectorTraits::Attributes::DstIp>(collectorAhkV6)
          .value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamCollector, TruncateSize, collectorV6->attributes()),
      std::get<std::optional<SaiTamCollectorTraits::Attributes::TruncateSize>>(
          collectorAhkV6)
          .value()
          .value());
  EXPECT_EQ(
      GET_ATTR(TamCollector, Transport, collectorV6->attributes()),
      std::get<SaiTamCollectorTraits::Attributes::Transport>(collectorAhkV6)
          .value());
  EXPECT_EQ(
      GET_OPT_ATTR(TamCollector, DscpValue, collectorV6->attributes()),
      std::get<std::optional<SaiTamCollectorTraits::Attributes::DscpValue>>(
          collectorAhkV6)
          .value()
          .value());

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
      GET_OPT_ATTR(TamEvent, SwitchEventType, event->attributes()),
      std::get<std::optional<SaiTamEventTraits::Attributes::SwitchEventType>>(
          eventAhk)
          .value()
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

  auto transportAhk = tamTransportTraits();
  auto transport =
      s.get<SaiTamTransportTraits>().setObject(transportAhk, transportAhk);

  auto collectorAhkV4 =
      tamCollectorTraits(transport->adapterKey(), true /* ipV4 */);
  auto collectorV4 =
      s.get<SaiTamCollectorTraits>().setObject(collectorAhkV4, collectorAhkV4);

  auto collectorAhkV6 =
      tamCollectorTraits(transport->adapterKey(), false /* ipV4 */);
  auto collectorV6 =
      s.get<SaiTamCollectorTraits>().setObject(collectorAhkV6, collectorAhkV6);

  auto tamAhk = tamTraits(event->adapterKey());
  auto tam = s.get<SaiTamTraits>().setObject(tamAhk, tamAhk);

  std::vector<sai_int32_t> newEvents = {4, 5, 6};
  std::get<std::optional<SaiTamEventTraits::Attributes::SwitchEventType>>(
      eventAhk) = newEvents;
  auto updatedEvent = s.get<SaiTamEventTraits>().setObject(eventAhk, eventAhk);
  EXPECT_EQ(
      GET_OPT_ATTR(TamEvent, SwitchEventType, updatedEvent->attributes()),
      newEvents);

  std::get<SaiTamTransportTraits::Attributes::DstPort>(transportAhk) = 10003;
  auto updatedTransport =
      s.get<SaiTamTransportTraits>().setObject(transportAhk, transportAhk);
  EXPECT_EQ(
      GET_ATTR(TamTransport, DstPort, updatedTransport->attributes()), 10003);

  std::get<std::optional<SaiTamCollectorTraits::Attributes::TruncateSize>>(
      collectorAhkV4) = 256;
  auto updatedCollectorV4 =
      s.get<SaiTamCollectorTraits>().setObject(collectorAhkV4, collectorAhkV4);
  EXPECT_EQ(
      GET_OPT_ATTR(
          TamCollector, TruncateSize, updatedCollectorV4->attributes()),
      256);

  std::get<std::optional<SaiTamCollectorTraits::Attributes::DscpValue>>(
      collectorAhkV6) = 2;
  auto updatedCollectorV6 =
      s.get<SaiTamCollectorTraits>().setObject(collectorAhkV6, collectorAhkV6);
  EXPECT_EQ(
      GET_OPT_ATTR(TamCollector, DscpValue, updatedCollectorV6->attributes()),
      2);
}
