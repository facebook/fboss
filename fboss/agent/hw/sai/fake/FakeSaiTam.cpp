// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/fake/FakeSaiTam.h"

#include "fboss/agent/hw/sai/fake/FakeSai.h"

namespace {

using facebook::fboss::FakeSai;

sai_status_t create_tam(
    sai_object_id_t* id,
    sai_object_id_t /*switch_id*/,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  std::vector<sai_object_id_t> events;
  std::vector<sai_int32_t> bindpoints;
  for (auto i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_TAM_ATTR_EVENT_OBJECTS_LIST:
        std::copy(
            attr_list[i].value.objlist.list,
            attr_list[i].value.objlist.list + attr_list[i].value.objlist.count,
            std::back_inserter(events));
        break;

      case SAI_TAM_ATTR_TAM_BIND_POINT_TYPE_LIST:
        std::copy(
            attr_list[i].value.s32list.list,
            attr_list[i].value.s32list.list + attr_list[i].value.s32list.count,
            std::back_inserter(bindpoints));
        break;

      default:
        return SAI_STATUS_ATTR_NOT_SUPPORTED_0 + i;
    }
  }
  auto fs = FakeSai::getInstance();
  *id = fs->tamManager.create(events, bindpoints);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_tam(sai_object_id_t id) {
  auto fs = FakeSai::getInstance();
  fs->tamManager.remove(id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_tam_attribute(
    sai_object_id_t id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto& tam = fs->tamManager.get(id);
  for (auto i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_TAM_ATTR_EVENT_OBJECTS_LIST:
        if (attr_list[i].value.objlist.count < tam.events_.size()) {
          attr_list[i].value.objlist.count = tam.events_.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        for (auto j = 0; j < attr_list[i].value.objlist.count; j++) {
          attr_list[i].value.objlist.list[j] = tam.events_[j];
        }
        break;

      case SAI_TAM_ATTR_TAM_BIND_POINT_TYPE_LIST:
        if (attr_list[i].value.s32list.count < tam.bindpoints_.size()) {
          attr_list[i].value.objlist.count = tam.bindpoints_.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        for (auto j = 0; j < attr_list[i].value.s32list.count; j++) {
          attr_list[i].value.s32list.list[j] = tam.bindpoints_[j];
        }
        break;

      default:
        return SAI_STATUS_ATTR_NOT_SUPPORTED_0 + i;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_tam_attribute(
    sai_object_id_t id,
    const sai_attribute_t* attr_list) {
  std::vector<sai_object_id_t> events;
  std::vector<sai_int32_t> bindpoints;
  switch (attr_list[0].id) {
    case SAI_TAM_ATTR_EVENT_OBJECTS_LIST:
      std::copy(
          attr_list[0].value.objlist.list,
          attr_list[0].value.objlist.list + attr_list[0].value.objlist.count,
          std::back_inserter(events));
      break;

    case SAI_TAM_ATTR_TAM_BIND_POINT_TYPE_LIST:
      std::copy(
          attr_list[0].value.s32list.list,
          attr_list[0].value.s32list.list + attr_list[0].value.s32list.count,
          std::back_inserter(bindpoints));
      break;
    default:
      return SAI_STATUS_ATTR_NOT_SUPPORTED_0;
  }
  try {
    auto fs = FakeSai::getInstance();
    auto& tam = fs->tamManager.get(id);
    tam.events_ = events;
    tam.bindpoints_ = bindpoints;
  } catch (...) {
    return SAI_STATUS_ITEM_NOT_FOUND;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_tam_event(
    sai_object_id_t* id,
    sai_object_id_t /*switch_id*/,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  sai_int32_t eventType{};
  std::vector<sai_object_id_t> actions{};
  std::vector<sai_object_id_t> collectors{};
  std::vector<sai_int32_t> switchEvents{};

  for (auto i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_TAM_EVENT_ATTR_TYPE:
        eventType = attr_list[i].value.s32;
        break;

      case SAI_TAM_EVENT_ATTR_ACTION_LIST:
        std::copy(
            attr_list[i].value.objlist.list,
            attr_list[i].value.objlist.list + attr_list[i].value.objlist.count,
            std::back_inserter(actions));
        break;

      case SAI_TAM_EVENT_ATTR_COLLECTOR_LIST:
        std::copy(
            attr_list[i].value.objlist.list,
            attr_list[i].value.objlist.list + attr_list[i].value.objlist.count,
            std::back_inserter(collectors));
        break;

      case SAI_TAM_EVENT_ATTR_FAKE_SWITCH_EVENT_TYPE:
        std::copy(
            attr_list[i].value.s32list.list,
            attr_list[i].value.s32list.list + attr_list[i].value.s32list.count,
            std::back_inserter(switchEvents));
        break;

      default:
        return SAI_STATUS_ATTR_NOT_SUPPORTED_0 + i;
    }
  }
  auto fs = FakeSai::getInstance();
  *id =
      fs->tamEventManager.create(eventType, actions, collectors, switchEvents);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_tam_event(sai_object_id_t id) {
  auto fs = FakeSai::getInstance();
  fs->tamEventManager.remove(id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_tam_event_attribute(
    sai_object_id_t id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto& eventAction = fs->tamEventManager.get(id);
  for (auto i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_TAM_EVENT_ATTR_TYPE:
        attr_list[i].value.s32 = eventAction.eventType_;
        break;

      case SAI_TAM_EVENT_ATTR_ACTION_LIST:
        if (attr_list[i].value.objlist.count < eventAction.actions_.size()) {
          attr_list[i].value.objlist.count = eventAction.actions_.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        for (auto j = 0; j < attr_list[i].value.objlist.count; j++) {
          attr_list[i].value.objlist.list[j] = eventAction.actions_[j];
        }
        break;

      case SAI_TAM_EVENT_ATTR_COLLECTOR_LIST:
        if (attr_list[i].value.objlist.count < eventAction.collectors_.size()) {
          attr_list[i].value.objlist.count = eventAction.collectors_.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        for (auto j = 0; j < attr_list[i].value.objlist.count; j++) {
          attr_list[i].value.objlist.list[j] = eventAction.collectors_[j];
        }
        break;

      case SAI_TAM_EVENT_ATTR_FAKE_SWITCH_EVENT_TYPE:
        if (attr_list[i].value.s32list.count <
            eventAction.switchEvents_.size()) {
          attr_list[i].value.s32list.count = eventAction.switchEvents_.size();
          return SAI_STATUS_BUFFER_OVERFLOW;
        }
        for (auto j = 0; j < attr_list[i].value.s32list.count; j++) {
          attr_list[i].value.s32list.list[j] = eventAction.switchEvents_[j];
        }
        break;

      default:
        return SAI_STATUS_ATTR_NOT_SUPPORTED_0 + i;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_tam_event_attribute(
    sai_object_id_t id,
    const sai_attribute_t* attr_list) {
  sai_int32_t eventType{};
  std::vector<sai_object_id_t> actions{};
  std::vector<sai_object_id_t> collectors{};
  std::vector<sai_int32_t> switchEvents{};

  switch (attr_list[0].id) {
    case SAI_TAM_EVENT_ATTR_TYPE:
      eventType = attr_list[0].value.s32;
      break;

    case SAI_TAM_EVENT_ATTR_ACTION_LIST:
      std::copy(
          attr_list[0].value.objlist.list,
          attr_list[0].value.objlist.list + attr_list[0].value.objlist.count,
          std::back_inserter(actions));
      break;

    case SAI_TAM_EVENT_ATTR_COLLECTOR_LIST:
      std::copy(
          attr_list[0].value.objlist.list,
          attr_list[0].value.objlist.list + attr_list[0].value.objlist.count,
          std::back_inserter(collectors));
      break;

    case SAI_TAM_EVENT_ATTR_FAKE_SWITCH_EVENT_TYPE:
      std::copy(
          attr_list[0].value.s32list.list,
          attr_list[0].value.s32list.list + attr_list[0].value.s32list.count,
          std::back_inserter(switchEvents));
      break;

    default:
      return SAI_STATUS_ATTR_NOT_SUPPORTED_0;
  }
  try {
    auto fs = FakeSai::getInstance();
    auto& tamEvent = fs->tamEventManager.get(id);
    tamEvent.eventType_ = eventType;
    tamEvent.actions_ = actions;
    tamEvent.collectors_ = collectors;
    tamEvent.switchEvents_ = switchEvents;
  } catch (...) {
    return SAI_STATUS_ITEM_NOT_FOUND;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_tam_event_action(
    sai_object_id_t* id,
    sai_object_id_t /*switch_id*/,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  sai_object_id_t reportType{};
  for (auto i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_TAM_EVENT_ACTION_ATTR_REPORT_TYPE:
        reportType = attr_list[i].value.oid;
        break;
      default:
        return SAI_STATUS_ATTR_NOT_SUPPORTED_0 + i;
    }
  }
  auto fs = FakeSai::getInstance();
  *id = fs->tamEventActionManager.create(reportType);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_tam_event_action(sai_object_id_t id) {
  auto fs = FakeSai::getInstance();
  fs->tamEventActionManager.remove(id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_tam_event_action_attribute(
    sai_object_id_t id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  auto& eventAction = fs->tamEventActionManager.get(id);
  for (auto i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_TAM_EVENT_ACTION_ATTR_REPORT_TYPE:
        attr_list[i].value.oid = eventAction.report_;
        break;
      default:
        return SAI_STATUS_ATTR_NOT_SUPPORTED_0 + i;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_tam_event_action_attribute(
    sai_object_id_t id,
    const sai_attribute_t* attr_list) {
  sai_object_id_t reportType{};
  switch (attr_list[0].id) {
    case SAI_TAM_EVENT_ACTION_ATTR_REPORT_TYPE:
      reportType = attr_list[0].value.oid;
      break;
    default:
      return SAI_STATUS_ATTR_NOT_SUPPORTED_0;
  }
  try {
    auto fs = FakeSai::getInstance();
    auto& tamEventAction = fs->tamEventActionManager.get(id);
    tamEventAction.report_ = reportType;
  } catch (...) {
    return SAI_STATUS_ITEM_NOT_FOUND;
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t create_tam_report(
    sai_object_id_t* id,
    sai_object_id_t /*switch_id*/,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  sai_int32_t type;
  for (auto i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_TAM_REPORT_ATTR_TYPE:
        type = attr_list[i].value.s32;
        break;
      default:
        return SAI_STATUS_ATTR_NOT_SUPPORTED_0 + i;
    }
  }
  auto fs = FakeSai::getInstance();
  *id = fs->tamReportManager.create(type);
  return SAI_STATUS_SUCCESS;
}

sai_status_t remove_tam_report(sai_object_id_t id) {
  auto fs = FakeSai::getInstance();
  fs->tamReportManager.remove(id);
  return SAI_STATUS_SUCCESS;
}

sai_status_t get_tam_report_attribute(
    sai_object_id_t id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  auto fs = FakeSai::getInstance();
  const auto& tamReport = fs->tamReportManager.get(id);
  for (auto i = 0; i < attr_count; i++) {
    switch (attr_list[i].id) {
      case SAI_TAM_REPORT_ATTR_TYPE:
        attr_list[i].value.s32 = tamReport.type_;
        break;
      default:
        return SAI_STATUS_ATTR_NOT_SUPPORTED_0 + i;
    }
  }
  return SAI_STATUS_SUCCESS;
}

sai_status_t set_tam_report_attribute(
    sai_object_id_t id,
    const sai_attribute_t* attr_list) {
  sai_int32_t type;
  switch (attr_list[0].id) {
    case SAI_TAM_REPORT_ATTR_TYPE:
      type = attr_list[0].value.s32;
      break;
    default:
      return SAI_STATUS_ATTR_NOT_SUPPORTED_0;
  }
  try {
    auto fs = FakeSai::getInstance();
    auto& tamReport = fs->tamReportManager.get(id);
    tamReport.type_ = type;
  } catch (...) {
    return SAI_STATUS_ITEM_NOT_FOUND;
  }
  return SAI_STATUS_SUCCESS;
}
} // namespace

namespace facebook::fboss {
static sai_tam_api_t _tam_api;

void populate_tam_api(sai_tam_api_t** tam_api) {
  _tam_api.create_tam = &create_tam;
  _tam_api.remove_tam = &remove_tam;
  _tam_api.set_tam_attribute = &set_tam_attribute;
  _tam_api.get_tam_attribute = &get_tam_attribute;

  _tam_api.create_tam_event = &create_tam_event;
  _tam_api.remove_tam_event = &remove_tam_event;
  _tam_api.set_tam_event_attribute = &set_tam_event_attribute;
  _tam_api.get_tam_event_attribute = &get_tam_event_attribute;

  _tam_api.create_tam_event_action = &create_tam_event_action;
  _tam_api.remove_tam_event_action = &remove_tam_event_action;
  _tam_api.set_tam_event_action_attribute = &set_tam_event_action_attribute;
  _tam_api.get_tam_event_action_attribute = &get_tam_event_action_attribute;

  _tam_api.create_tam_report = &create_tam_report;
  _tam_api.remove_tam_report = &remove_tam_report;
  _tam_api.set_tam_report_attribute = &set_tam_report_attribute;
  _tam_api.get_tam_report_attribute = &get_tam_report_attribute;

  *tam_api = &_tam_api;
}

} // namespace facebook::fboss
