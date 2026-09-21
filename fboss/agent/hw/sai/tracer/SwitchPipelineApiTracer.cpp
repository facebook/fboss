/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/tracer/SwitchPipelineApiTracer.h"
#include "fboss/agent/hw/sai/api/SwitchPipelineApi.h"

#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
using folly::to;

namespace {

std::map<int32_t, std::pair<std::string, std::size_t>> _SwitchPipelineMap{
    SAI_ATTR_MAP(SwitchPipeline, Index),
};

void handleExtensionAttributes() {}

} // namespace

namespace facebook::fboss {

WRAP_CREATE_FUNC(
    switch_pipeline,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_SWITCH_PIPELINE),
    switchPipeline);
WRAP_REMOVE_FUNC(
    switch_pipeline,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_SWITCH_PIPELINE),
    switchPipeline);
WRAP_SET_ATTR_FUNC(
    switch_pipeline,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_SWITCH_PIPELINE),
    switchPipeline);
WRAP_GET_ATTR_FUNC(
    switch_pipeline,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_SWITCH_PIPELINE),
    switchPipeline);
WRAP_GET_STATS_FUNC(
    switch_pipeline,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_SWITCH_PIPELINE),
    switchPipeline);
WRAP_GET_STATS_EXT_FUNC(
    switch_pipeline,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_SWITCH_PIPELINE),
    switchPipeline);
WRAP_CLEAR_STATS_FUNC(
    switch_pipeline,
    static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_SWITCH_PIPELINE),
    switchPipeline);

sai_switch_pipeline_api_t* wrappedSwitchPipelineApi() {
  handleExtensionAttributes();
  static sai_switch_pipeline_api_t switchPipelineWrappers;

  switchPipelineWrappers.create_switch_pipeline = &wrap_create_switch_pipeline;
  switchPipelineWrappers.remove_switch_pipeline = &wrap_remove_switch_pipeline;
  switchPipelineWrappers.set_switch_pipeline_attribute =
      &wrap_set_switch_pipeline_attribute;
  switchPipelineWrappers.get_switch_pipeline_attribute =
      &wrap_get_switch_pipeline_attribute;
  switchPipelineWrappers.get_switch_pipeline_stats =
      &wrap_get_switch_pipeline_stats;
  switchPipelineWrappers.get_switch_pipeline_stats_ext =
      &wrap_get_switch_pipeline_stats_ext;
  switchPipelineWrappers.clear_switch_pipeline_stats =
      &wrap_clear_switch_pipeline_stats;

  return &switchPipelineWrappers;
}

SET_SAI_ATTRIBUTES(SwitchPipeline)

} // namespace facebook::fboss

#endif
