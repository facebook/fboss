// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/tracer/TamApiTracer.h"

namespace facebook::fboss {

sai_status_t wrap_create_tam(
    sai_object_id_t* id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->create_tam(
      id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_tam(sai_object_id_t id) {
  return SaiTracer::getInstance()->tamApi_->remove_tam(id);
}

sai_status_t wrap_get_tam_attribute(
    sai_object_id_t id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->get_tam_attribute(
      id, attr_count, attr_list);
}

sai_status_t wrap_set_tam_attribute(
    sai_object_id_t id,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->set_tam_attribute(id, attr_list);
}

sai_status_t wrap_create_tam_event(
    sai_object_id_t* id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->create_tam_event(
      id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_tam_event(sai_object_id_t id) {
  return SaiTracer::getInstance()->tamApi_->remove_tam_event(id);
}

sai_status_t wrap_get_tam_event_attribute(
    sai_object_id_t id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->get_tam_event_attribute(
      id, attr_count, attr_list);
}

sai_status_t wrap_set_tam_event_attribute(
    sai_object_id_t id,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->set_tam_event_attribute(
      id, attr_list);
}

sai_status_t wrap_create_tam_event_action(
    sai_object_id_t* id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->create_tam_event_action(
      id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_tam_event_action(sai_object_id_t id) {
  return SaiTracer::getInstance()->tamApi_->remove_tam_event_action(id);
}

sai_status_t wrap_get_tam_event_action_attribute(
    sai_object_id_t id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->get_tam_event_action_attribute(
      id, attr_count, attr_list);
}

sai_status_t wrap_set_tam_event_action_attribute(
    sai_object_id_t id,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->set_tam_event_action_attribute(
      id, attr_list);
}

sai_status_t wrap_create_tam_report(
    sai_object_id_t* id,
    sai_object_id_t switch_id,
    uint32_t attr_count,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->create_tam_report(
      id, switch_id, attr_count, attr_list);
}

sai_status_t wrap_remove_tam_report(sai_object_id_t id) {
  return SaiTracer::getInstance()->tamApi_->remove_tam_report(id);
}

sai_status_t wrap_get_tam_report_attribute(
    sai_object_id_t id,
    uint32_t attr_count,
    sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->get_tam_report_attribute(
      id, attr_count, attr_list);
}

sai_status_t wrap_set_tam_report_attribute(
    sai_object_id_t id,
    const sai_attribute_t* attr_list) {
  return SaiTracer::getInstance()->tamApi_->set_tam_report_attribute(
      id, attr_list);
}

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
} // namespace facebook::fboss
