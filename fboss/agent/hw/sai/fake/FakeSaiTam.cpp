// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/fake/FakeSaiTam.h"

namespace {

sai_status_t create_tam(
    sai_object_id_t* /*id*/,
    sai_object_id_t /*switch_id*/,
    uint32_t /*attr_count*/,
    const sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t remove_tam(sai_object_id_t /*id*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t get_tam_attribute(
    sai_object_id_t /*id*/,
    uint32_t /*attr_count*/,
    sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t set_tam_attribute(
    sai_object_id_t /*id*/,
    const sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t create_tam_event(
    sai_object_id_t* /*id*/,
    sai_object_id_t /*switch_id*/,
    uint32_t /*attr_count*/,
    const sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t remove_tam_event(sai_object_id_t /*id*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t get_tam_event_attribute(
    sai_object_id_t /*id*/,
    uint32_t /*attr_count*/,
    sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t set_tam_event_attribute(
    sai_object_id_t /*id*/,
    const sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t create_tam_event_action(
    sai_object_id_t* /*id*/,
    sai_object_id_t /*switch_id*/,
    uint32_t /*attr_count*/,
    const sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t remove_tam_event_action(sai_object_id_t /*id*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t get_tam_event_action_attribute(
    sai_object_id_t /*id*/,
    uint32_t /*attr_count*/,
    sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t set_tam_event_action_attribute(
    sai_object_id_t /*id*/,
    const sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t create_tam_report(
    sai_object_id_t* /*id*/,
    sai_object_id_t /*switch_id*/,
    uint32_t /*attr_count*/,
    const sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t remove_tam_report(sai_object_id_t /*id*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t get_tam_report_attribute(
    sai_object_id_t /*id*/,
    uint32_t /*attr_count*/,
    sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
}

sai_status_t set_tam_report_attribute(
    sai_object_id_t /*id*/,
    const sai_attribute_t* /*attr_list*/) {
  return SAI_STATUS_NOT_IMPLEMENTED;
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
