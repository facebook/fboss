// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/tracer/TamApiTracer.h"
#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

namespace facebook::fboss {

WRAP_CREATE_FUNC(tam, SAI_OBJECT_TYPE_TAM, tam);
WRAP_REMOVE_FUNC(tam, SAI_OBJECT_TYPE_TAM, tam);
WRAP_SET_ATTR_FUNC(tam, SAI_OBJECT_TYPE_TAM, tam);
WRAP_GET_ATTR_FUNC(tam, SAI_OBJECT_TYPE_TAM, tam);

WRAP_CREATE_FUNC(tam_event, SAI_OBJECT_TYPE_TAM_EVENT, tam);
WRAP_REMOVE_FUNC(tam_event, SAI_OBJECT_TYPE_TAM_EVENT, tam);
WRAP_SET_ATTR_FUNC(tam_event, SAI_OBJECT_TYPE_TAM_EVENT, tam);
WRAP_GET_ATTR_FUNC(tam_event, SAI_OBJECT_TYPE_TAM_EVENT, tam);

WRAP_CREATE_FUNC(tam_event_action, SAI_OBJECT_TYPE_TAM_EVENT_ACTION, tam);
WRAP_REMOVE_FUNC(tam_event_action, SAI_OBJECT_TYPE_TAM_EVENT_ACTION, tam);
WRAP_SET_ATTR_FUNC(tam_event_action, SAI_OBJECT_TYPE_TAM_EVENT_ACTION, tam);
WRAP_GET_ATTR_FUNC(tam_event_action, SAI_OBJECT_TYPE_TAM_EVENT_ACTION, tam);

WRAP_CREATE_FUNC(tam_report, SAI_OBJECT_TYPE_TAM_REPORT, tam);
WRAP_REMOVE_FUNC(tam_report, SAI_OBJECT_TYPE_TAM_REPORT, tam);
WRAP_SET_ATTR_FUNC(tam_report, SAI_OBJECT_TYPE_TAM_REPORT, tam);
WRAP_GET_ATTR_FUNC(tam_report, SAI_OBJECT_TYPE_TAM_REPORT, tam);

sai_tam_api_t* wrappedTamApi() {
  static sai_tam_api_t tamWrappers;
  tamWrappers.create_tam = &wrap_create_tam;
  tamWrappers.remove_tam = &wrap_remove_tam;
  tamWrappers.set_tam_attribute = &wrap_set_tam_attribute;
  tamWrappers.get_tam_attribute = &wrap_get_tam_attribute;

  tamWrappers.create_tam_event = &wrap_create_tam_event;
  tamWrappers.remove_tam_event = &wrap_remove_tam_event;
  tamWrappers.set_tam_event_attribute = &wrap_set_tam_event_attribute;
  tamWrappers.get_tam_event_attribute = &wrap_get_tam_event_attribute;

  tamWrappers.create_tam_event_action = &wrap_create_tam_event_action;
  tamWrappers.remove_tam_event_action = &wrap_remove_tam_event_action;
  tamWrappers.set_tam_event_action_attribute =
      &wrap_set_tam_event_action_attribute;
  tamWrappers.get_tam_event_action_attribute =
      &wrap_get_tam_event_action_attribute;

  tamWrappers.create_tam_report = &wrap_create_tam_report;
  tamWrappers.remove_tam_report = &wrap_remove_tam_report;
  tamWrappers.set_tam_report_attribute = &wrap_set_tam_report_attribute;
  tamWrappers.get_tam_report_attribute = &wrap_get_tam_report_attribute;

  return &tamWrappers;
}

void setTamAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_TAM_ATTR_EVENT_OBJECTS_LIST:
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;
      case SAI_TAM_ATTR_TAM_BIND_POINT_TYPE_LIST:
        s32ListAttr(attr_list, i, listCount++, attrLines);
        break;
      default:
        break;
    }
  }
}

void setTamEventAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  uint32_t listCount = 0;

  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_TAM_EVENT_ATTR_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      case SAI_TAM_EVENT_ATTR_ACTION_LIST:
      case SAI_TAM_EVENT_ATTR_COLLECTOR_LIST:
        oidListAttr(attr_list, i, listCount++, attrLines);
        break;
      default:
        // Handle extension attributes
        auto switchEventTypeId = facebook::fboss::SaiTamEventTraits::
            Attributes::SwitchEventType::optionalExtensionAttributeId();
        auto switchEventId = facebook::fboss::SaiTamEventTraits::Attributes::
            SwitchEventId::optionalExtensionAttributeId();
        if (switchEventTypeId.has_value() &&
            attr_list[i].id == switchEventTypeId.value()) {
          s32ListAttr(attr_list, i, listCount++, attrLines);
        } else if (
            switchEventId.has_value() &&
            attr_list[i].id == switchEventId.value()) {
          attrLines.push_back(u32Attr(attr_list, i));
        }
        break;
    }
  }
}

void setTamEventActionAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_TAM_EVENT_ACTION_ATTR_REPORT_TYPE:
        attrLines.push_back(oidAttr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

void setTamReportAttributes(
    const sai_attribute_t* attr_list,
    uint32_t attr_count,
    std::vector<std::string>& attrLines) {
  for (int i = 0; i < attr_count; ++i) {
    switch (attr_list[i].id) {
      case SAI_TAM_REPORT_ATTR_TYPE:
        attrLines.push_back(s32Attr(attr_list, i));
        break;
      default:
        break;
    }
  }
}

} // namespace facebook::fboss
