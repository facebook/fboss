// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/fake/FakeManager.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class FakeSaiTam {
 public:
  FakeSaiTam(
      const std::vector<sai_object_id_t>& events,
      const std::vector<sai_int32_t>& bindpoints)
      : events_(events), bindpoints_(bindpoints) {}

  sai_object_id_t id;
  std::vector<sai_object_id_t> events_;
  std::vector<sai_int32_t> bindpoints_;
};

class FakeSaiTamEvent {
 public:
  FakeSaiTamEvent(
      sai_int32_t eventType,
      const std::vector<sai_object_id_t>& actions,
      const std::vector<sai_object_id_t>& collectors,
      const std::vector<sai_int32_t>& switchEvents,
      sai_int32_t deviceId,
      sai_int32_t eventId,
      std::vector<sai_object_id_t> extensionsCollectorList,
      std::vector<sai_int32_t> packetDropTypeMmu,
      sai_object_id_t agingGroup)
      : eventType_(eventType),
        actions_(actions),
        collectors_(collectors),
        switchEvents_(switchEvents),
        deviceId_(deviceId),
        eventId_(eventId),
        extensionsCollectorList_(std::move(extensionsCollectorList)),
        packetDropTypeMmu_(std::move(packetDropTypeMmu)),
        agingGroup_(agingGroup) {}
  sai_object_id_t id;
  sai_int32_t eventType_;
  std::vector<sai_object_id_t> actions_;
  std::vector<sai_object_id_t> collectors_;
  std::vector<sai_int32_t> switchEvents_;
  sai_int32_t deviceId_;
  sai_int32_t eventId_;
  std::vector<sai_object_id_t> extensionsCollectorList_;
  std::vector<sai_int32_t> packetDropTypeMmu_;
  sai_object_id_t agingGroup_;
};

class FakeSaiTamEventAction {
 public:
  explicit FakeSaiTamEventAction(sai_object_id_t report) : report_(report) {}
  sai_object_id_t id;
  sai_object_id_t report_;
};

class FakeSaiTamReport {
 public:
  explicit FakeSaiTamReport(sai_int32_t type) : type_(type) {}
  sai_object_id_t id;
  sai_int32_t type_;
};

class FakeSaiTamTransport {
 public:
  FakeSaiTamTransport(
      sai_int32_t transportType,
      sai_uint32_t srcPort,
      sai_uint32_t dstPort,
      sai_uint32_t mtu,
      std::optional<folly::MacAddress> srcMac,
      std::optional<folly::MacAddress> dstMac)
      : transportType_(transportType),
        srcPort_(srcPort),
        dstPort_(dstPort),
        mtu_(mtu),
        srcMac_(srcMac),
        dstMac_(dstMac) {}
  sai_object_id_t id;
  sai_int32_t transportType_;
  sai_uint32_t srcPort_;
  sai_uint32_t dstPort_;
  sai_uint32_t mtu_;
  std::optional<folly::MacAddress> srcMac_;
  std::optional<folly::MacAddress> dstMac_;
};

class FakeSaiTamCollector {
 public:
  FakeSaiTamCollector(
      const folly::IPAddress& srcIp,
      const folly::IPAddress& dstIp,
      std::optional<sai_uint16_t> truncateSize,
      sai_object_id_t transport,
      std::optional<sai_uint8_t> dscp)
      : srcIp_(srcIp),
        dstIp_(dstIp),
        truncateSize_(truncateSize),
        transport_(transport),
        dscp_(dscp) {}
  sai_object_id_t id;
  folly::IPAddress srcIp_;
  folly::IPAddress dstIp_;
  std::optional<sai_uint16_t> truncateSize_;
  sai_object_id_t transport_;
  std::optional<sai_uint8_t> dscp_;
};

using FakeTamManager = FakeManager<sai_object_id_t, FakeSaiTam>;
using FakeTamEventManager = FakeManager<sai_object_id_t, FakeSaiTamEvent>;
using FakeTamEventActionManager =
    FakeManager<sai_object_id_t, FakeSaiTamEventAction>;
using FakeTamReportManager = FakeManager<sai_object_id_t, FakeSaiTamReport>;
using FakeTamTransportManager =
    FakeManager<sai_object_id_t, FakeSaiTamTransport>;
using FakeTamCollectorManager =
    FakeManager<sai_object_id_t, FakeSaiTamCollector>;

void populate_tam_api(sai_tam_api_t** tam_api);

} // namespace facebook::fboss
