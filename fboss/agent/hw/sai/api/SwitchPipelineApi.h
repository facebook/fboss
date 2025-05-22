// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)

extern "C" {
#include <sai.h>
#include <saiextensions.h>
#ifndef IS_OSS_BRCM_SAI
#include <experimental/saiexperimentalswitchpipeline.h>
#else
#include <saiexperimentalswitchpipeline.h>
#endif
}

namespace facebook::fboss {

class SwitchPipelineApi;

struct SaiSwitchPipelineTraits {
  // IMPORTANT: Cast to sai_object_type_t so that all our infrastructure works.
  static constexpr sai_object_type_t ObjectType =
      static_cast<sai_object_type_t>(SAI_OBJECT_TYPE_SWITCH_PIPELINE);
  using SaiApiT = SwitchPipelineApi;
  struct Attributes {
    using EnumType = sai_switch_pipeline_attr_t;
    using Index = SaiAttribute<
        EnumType,
        SAI_SWITCH_PIPELINE_ATTR_INDEX,
        sai_uint32_t,
        SaiIntDefault<sai_uint32_t>>;
  };
  using AdapterKey = SwitchPipelineSaiId;
  using AdapterHostKey = Attributes::Index;
  using CreateAttributes = std::tuple<Attributes::Index>;

  static const std::vector<sai_stat_id_t>& watermarkLevels() {
    static const std::vector<sai_stat_id_t> stats{
        SAI_SWITCH_PIPELINE_STAT_RX_WATERMARK_LEVELS,
        SAI_SWITCH_PIPELINE_STAT_TX_WATERMARK_LEVELS,
    };
    return stats;
  }

  static const std::vector<sai_stat_id_t>& currOccupancyBytes() {
    static const std::vector<sai_stat_id_t> stats{
        SAI_SWITCH_PIPELINE_STAT_CURR_OCCUPANCY_BYTES,
    };
    return stats;
  }

  static const std::vector<sai_stat_id_t>& rxCells() {
    static const std::vector<sai_stat_id_t> stats{
        SAI_SWITCH_PIPELINE_STAT_RX_CELLS,
    };
    return stats;
  }

  static const std::vector<sai_stat_id_t>& txCells() {
    static const std::vector<sai_stat_id_t> stats{
        SAI_SWITCH_PIPELINE_STAT_TX_CELLS,
    };
    return stats;
  }

  static const std::vector<sai_stat_id_t>& globalDrop() {
    static const std::vector<sai_stat_id_t> stats{
        SAI_SWITCH_PIPELINE_STAT_GLOBAL_DROP,
    };
    return stats;
  }
};

SAI_ATTRIBUTE_NAME(SwitchPipeline, Index)

template <>
struct SaiObjectHasStats<SaiSwitchPipelineTraits> : public std::true_type {};

class SwitchPipelineApi : public SaiApi<SwitchPipelineApi> {
 public:
  static constexpr sai_api_t ApiType =
      static_cast<sai_api_t>(SAI_API_SWITCH_PIPELINE);
  SwitchPipelineApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(
        status, ApiType, "Failed to query for switch pipeline api");
  }

 private:
  sai_status_t _create(
      SwitchPipelineSaiId* id,
      sai_object_id_t switch_pipeline_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_switch_pipeline(
        rawSaiId(id), switch_pipeline_id, count, attr_list);
  }

  sai_status_t _remove(SwitchPipelineSaiId id) const {
    return api_->remove_switch_pipeline(id);
  }

  sai_status_t _getAttribute(SwitchPipelineSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_switch_pipeline_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(
      SwitchPipelineSaiId id,
      const sai_attribute_t* attr) const {
    return api_->set_switch_pipeline_attribute(id, attr);
  }

  sai_status_t _getStats(
      SwitchPipelineSaiId id,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids,
      sai_stats_mode_t mode,
      uint64_t* counters) const {
    return mode == SAI_STATS_MODE_READ
        ? api_->get_switch_pipeline_stats(
              id, num_of_counters, counter_ids, counters)
        : api_->get_switch_pipeline_stats_ext(
              id, num_of_counters, counter_ids, mode, counters);
  }

  sai_status_t _clearStats(
      SwitchPipelineSaiId id,
      uint32_t num_of_counters,
      const sai_stat_id_t* counter_ids) const {
    return api_->clear_switch_pipeline_stats(id, num_of_counters, counter_ids);
  }

  sai_switch_pipeline_api_t* api_;
  friend class SaiApi<SwitchPipelineApi>;
};

} // namespace facebook::fboss

#endif
