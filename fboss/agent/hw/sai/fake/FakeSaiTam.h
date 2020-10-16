// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/fake/FakeManager.h"

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
      const std::vector<sai_int32_t>& switchEvents)
      : eventType_(eventType),
        actions_(actions),
        collectors_(collectors),
        switchEvents_(switchEvents) {}
  sai_object_id_t id;
  sai_int32_t eventType_;
  std::vector<sai_object_id_t> actions_;
  std::vector<sai_object_id_t> collectors_;
  std::vector<sai_int32_t> switchEvents_;
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

using FakeTamManager = FakeManager<sai_object_id_t, FakeSaiTam>;
using FakeTamEventManager = FakeManager<sai_object_id_t, FakeSaiTamEvent>;
using FakeTamEventActionManager =
    FakeManager<sai_object_id_t, FakeSaiTamEventAction>;
using FakeTamReportManager = FakeManager<sai_object_id_t, FakeSaiTamReport>;

void populate_tam_api(sai_tam_api_t** tam_api);

} // namespace facebook::fboss
