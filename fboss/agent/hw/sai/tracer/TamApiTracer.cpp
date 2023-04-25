/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/TamApiTracer.h"
#include <typeindex>
#include <utility>

#include "fboss/agent/hw/sai/api/TamApi.h"
#include "fboss/agent/hw/sai/tracer/Utils.h"

using folly::to;

namespace {
std::map<int32_t, std::pair<std::string, std::size_t>> _TamMap{
    SAI_ATTR_MAP(Tam, EventObjectList),
    SAI_ATTR_MAP(Tam, TamBindPointList),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _TamEventMap{
    SAI_ATTR_MAP(TamEvent, Type),
    SAI_ATTR_MAP(TamEvent, ActionList),
    SAI_ATTR_MAP(TamEvent, CollectorList),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _TamEventActionMap{
    SAI_ATTR_MAP(TamEventAction, ReportType),
};

std::map<int32_t, std::pair<std::string, std::size_t>> _TamReportMap{
    SAI_ATTR_MAP(TamReport, Type),
};

void handleExtensionAttributes() {
  SAI_EXT_ATTR_MAP(TamEvent, SwitchEventType)
  SAI_EXT_ATTR_MAP(TamEvent, SwitchEventId)
}

} // namespace

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
  handleExtensionAttributes();
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

SET_SAI_ATTRIBUTES(Tam)
SET_SAI_ATTRIBUTES(TamEvent)
SET_SAI_ATTRIBUTES(TamEventAction)
SET_SAI_ATTRIBUTES(TamReport)

} // namespace facebook::fboss
