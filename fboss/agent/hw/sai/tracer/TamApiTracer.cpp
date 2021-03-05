// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/tracer/TamApiTracer.h"

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

// TODO(zecheng): Set Tam attributes

} // namespace facebook::fboss
