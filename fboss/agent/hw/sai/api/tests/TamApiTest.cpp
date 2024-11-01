// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/MacAddress.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

using namespace facebook::fboss;

class TamApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    tamApi = std::make_unique<TamApi>();
  }

 protected:
  static constexpr auto switchId = 0;
  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<TamApi> tamApi;
};

TEST_F(TamApiTest, TamCollectorV4) {
  folly::IPAddress srcIp{"1.1.1.1"};
  folly::IPAddress dstIp{"2.2.2.2"};
  sai_object_id_t transportSaiObjectId = 100;

  SaiTamCollectorTraits::CreateAttributes collectorAttr;
  std::get<SaiTamCollectorTraits::Attributes::SrcIp>(collectorAttr) = srcIp;
  std::get<SaiTamCollectorTraits::Attributes::DstIp>(collectorAttr) = dstIp;
  std::get<std::optional<SaiTamCollectorTraits::Attributes::TruncateSize>>(
      collectorAttr) = 128;
  std::get<SaiTamCollectorTraits::Attributes::Transport>(collectorAttr) =
      transportSaiObjectId;
  std::get<std::optional<SaiTamCollectorTraits::Attributes::DscpValue>>(
      collectorAttr) = 0;

  auto collectorSaiId =
      tamApi->create<SaiTamCollectorTraits>(collectorAttr, switchId);
  EXPECT_EQ(
      tamApi->getAttribute(
          collectorSaiId, SaiTamCollectorTraits::CreateAttributes{}),
      collectorAttr);
  tamApi->remove(collectorSaiId);
  EXPECT_THROW(
      tamApi->getAttribute(
          collectorSaiId, SaiTamCollectorTraits::CreateAttributes{}),
      std::exception);
}

TEST_F(TamApiTest, TamCollectorV6) {
  folly::IPAddress srcIp{"2401::1"};
  folly::IPAddress dstIp{"2401::2"};
  sai_object_id_t transportSaiObjectId = 100;

  SaiTamCollectorTraits::CreateAttributes collectorAttr;
  std::get<SaiTamCollectorTraits::Attributes::SrcIp>(collectorAttr) = srcIp;
  std::get<SaiTamCollectorTraits::Attributes::DstIp>(collectorAttr) = dstIp;
  std::get<std::optional<SaiTamCollectorTraits::Attributes::TruncateSize>>(
      collectorAttr) = 128;
  std::get<SaiTamCollectorTraits::Attributes::Transport>(collectorAttr) =
      transportSaiObjectId;
  std::get<std::optional<SaiTamCollectorTraits::Attributes::DscpValue>>(
      collectorAttr) = 0;

  auto collectorSaiId =
      tamApi->create<SaiTamCollectorTraits>(collectorAttr, switchId);
  EXPECT_EQ(
      tamApi->getAttribute(
          collectorSaiId, SaiTamCollectorTraits::CreateAttributes{}),
      collectorAttr);
  tamApi->remove(collectorSaiId);
  EXPECT_THROW(
      tamApi->getAttribute(
          collectorSaiId, SaiTamCollectorTraits::CreateAttributes{}),
      std::exception);
}

TEST_F(TamApiTest, TamTransport) {
  SaiTamTransportTraits::CreateAttributes transportAttr;
  std::get<SaiTamTransportTraits::Attributes::Type>(transportAttr) =
      SAI_TAM_TRANSPORT_TYPE_UDP;
  std::get<SaiTamTransportTraits::Attributes::SrcPort>(transportAttr) = 10001;
  std::get<SaiTamTransportTraits::Attributes::DstPort>(transportAttr) = 10002;
  std::get<SaiTamTransportTraits::Attributes::Mtu>(transportAttr) = 1500;
  std::get<std::optional<SaiTamTransportTraits::Attributes::SrcMacAddress>>(
      transportAttr) = folly::MacAddress("00:00:00:00:00:01");
  std::get<std::optional<SaiTamTransportTraits::Attributes::DstMacAddress>>(
      transportAttr) = folly::MacAddress("00:00:00:00:00:02");

  auto transportSaiId =
      tamApi->create<SaiTamTransportTraits>(transportAttr, switchId);
  EXPECT_EQ(
      tamApi->getAttribute(
          transportSaiId, SaiTamTransportTraits::CreateAttributes{}),
      transportAttr);
  tamApi->remove(transportSaiId);
  EXPECT_THROW(
      tamApi->getAttribute(
          transportSaiId, SaiTamTransportTraits::CreateAttributes{}),
      std::exception);
}

TEST_F(TamApiTest, TamReport) {
  SaiTamReportTraits::CreateAttributes reportAttr;
  // for call back
  std::get<SaiTamReportTraits::Attributes::Type>(reportAttr) =
      SAI_TAM_REPORT_TYPE_VENDOR_EXTN;
  auto reportSaiId = tamApi->create<SaiTamReportTraits>(reportAttr, switchId);
  EXPECT_EQ(
      tamApi->getAttribute(reportSaiId, SaiTamReportTraits::CreateAttributes{}),
      reportAttr);
  tamApi->remove(reportSaiId);
  EXPECT_THROW(
      tamApi->getAttribute(reportSaiId, SaiTamReportTraits::CreateAttributes{}),
      std::exception);
}

TEST_F(TamApiTest, TamEventAction) {
  SaiTamEventActionTraits::CreateAttributes eventActionAttr;
  std::get<SaiTamEventActionTraits::Attributes::ReportType>(eventActionAttr) =
      1; // object id of report
  auto eventActionSaiId =
      tamApi->create<SaiTamEventActionTraits>(eventActionAttr, switchId);
  EXPECT_EQ(
      tamApi->getAttribute(
          eventActionSaiId, SaiTamEventActionTraits::CreateAttributes{}),
      eventActionAttr);
  tamApi->remove(eventActionSaiId);
  EXPECT_THROW(
      tamApi->getAttribute(
          eventActionSaiId, SaiTamEventActionTraits::CreateAttributes{}),
      std::exception);
}

TEST_F(TamApiTest, TamEvent) {
  std::vector<sai_object_id_t> eventActions{100}; // object id of event action
  std::vector<sai_object_id_t> eventCollectors{SAI_NULL_OBJECT_ID};
  std::vector<sai_int32_t> eventTypes{1, 2, 3, 4}; // parity error e.g.

  sai_int32_t deviceId = 0;
  sai_int32_t eventId = 1;
  std::vector<sai_object_id_t> extensionsCollectorList{10};
  std::vector<sai_int32_t> packetDropTypeMmu = {3, 4};
  sai_object_id_t agingGroup = 20;

  SaiTamEventTraits::CreateAttributes eventAttr;
  std::get<SaiTamEventTraits::Attributes::Type>(eventAttr) =
      SAI_TAM_EVENT_TYPE_PACKET_DROP; // type of Event
  std::get<SaiTamEventTraits::Attributes::ActionList>(eventAttr) = eventActions;
  std::get<SaiTamEventTraits::Attributes::CollectorList>(eventAttr) =
      eventCollectors;
  std::get<std::optional<SaiTamEventTraits::Attributes::SwitchEventType>>(
      eventAttr) = eventTypes;
  std::get<std::optional<SaiTamEventTraits::Attributes::DeviceId>>(eventAttr) =
      deviceId;
  std::get<std::optional<SaiTamEventTraits::Attributes::SwitchEventId>>(
      eventAttr) = eventId;
  std::get<
      std::optional<SaiTamEventTraits::Attributes::ExtensionsCollectorList>>(
      eventAttr) = extensionsCollectorList;
  std::get<std::optional<SaiTamEventTraits::Attributes::PacketDropTypeMmu>>(
      eventAttr) = packetDropTypeMmu;
  std::get<std::optional<SaiTamEventTraits::Attributes::AgingGroup>>(
      eventAttr) = agingGroup;

  auto eventSaiId = tamApi->create<SaiTamEventTraits>(eventAttr, switchId);
  EXPECT_EQ(
      tamApi->getAttribute(eventSaiId, SaiTamEventTraits::CreateAttributes{}),
      eventAttr);
  tamApi->remove(eventSaiId);
  EXPECT_THROW(
      tamApi->getAttribute(eventSaiId, SaiTamEventTraits::CreateAttributes{}),
      std::exception);
}

TEST_F(TamApiTest, Tam) {
  std::vector<sai_object_id_t> tamEvents{100}; // object id of event
  std::vector<sai_int32_t> tamBindpoints{
      SAI_TAM_BIND_POINT_TYPE_SWITCH}; // bind point of tam

  SaiTamTraits::CreateAttributes tamAttr{tamEvents, tamBindpoints};

  auto tamSaiId = tamApi->create<SaiTamTraits>(tamAttr, switchId);
  EXPECT_EQ(
      tamApi->getAttribute(tamSaiId, SaiTamTraits::CreateAttributes{}),
      tamAttr);
  tamApi->remove(tamSaiId);
  EXPECT_THROW(
      tamApi->getAttribute(tamSaiId, SaiTamEventTraits::CreateAttributes{}),
      std::exception);
}
