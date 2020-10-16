// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/hw/sai/api/SaiApi.h"
#include "fboss/agent/hw/sai/api/SaiAttribute.h"
#include "fboss/agent/hw/sai/api/SaiAttributeDataTypes.h"
#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Types.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

class TamApi;

struct SaiTamReportTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_TAM_REPORT;
  using SaiApiT = TamApi;
  struct Attributes {
    using EnumType = sai_tam_report_attr_t;
    using Type = SaiAttribute<EnumType, SAI_TAM_REPORT_ATTR_TYPE, sai_int32_t>;
  };
  using AdapterKey = TamReportSaiId;
  using AdapterHostKey = std::tuple<Attributes::Type>;
  using CreateAttributes = std::tuple<Attributes::Type>;
};

SAI_ATTRIBUTE_NAME(TamReport, Type)

struct SaiTamEventActionTraits {
  static constexpr sai_object_type_t ObjectType =
      SAI_OBJECT_TYPE_TAM_EVENT_ACTION;
  using SaiApiT = TamApi;
  struct Attributes {
    using EnumType = sai_tam_event_action_attr_t;
    using ReportType = SaiAttribute<
        EnumType,
        SAI_TAM_EVENT_ACTION_ATTR_REPORT_TYPE,
        sai_object_id_t>;
  };
  using AdapterKey = TamEventActionSaiId;
  using AdapterHostKey = std::tuple<Attributes::ReportType>;
  using CreateAttributes = std::tuple<Attributes::ReportType>;
};

SAI_ATTRIBUTE_NAME(TamEventAction, ReportType)

struct SaiTamEventTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_TAM_EVENT;
  using SaiApiT = TamApi;
  struct Attributes {
    using EnumType = sai_tam_event_attr_t;
    using Type = SaiAttribute<EnumType, SAI_TAM_EVENT_ATTR_TYPE, sai_int32_t>;
    using ActionList = SaiAttribute<
        EnumType,
        SAI_TAM_EVENT_ATTR_ACTION_LIST,
        std::vector<sai_object_id_t>>;
    using CollectorList = SaiAttribute<
        EnumType,
        SAI_TAM_EVENT_ATTR_COLLECTOR_LIST,
        std::vector<sai_object_id_t>>;
    /* extension attributes */
    struct AttributeSwitchEventType {
      std::optional<sai_attr_id_t> operator()();
    };
    using SwitchEventType = SaiExtensionAttribute<
        std::vector<sai_int32_t>,
        AttributeSwitchEventType>;

    struct AttributeEventId {
      std::optional<sai_attr_id_t> operator()();
    };
    using SwitchEventId =
        SaiExtensionAttribute<std::vector<sai_int32_t>, AttributeEventId>;
  };
  using AdapterKey = TamEventSaiId;
  using AdapterHostKey = std::tuple<
      Attributes::Type,
      Attributes::ActionList,
      Attributes::CollectorList,
      Attributes::SwitchEventType>;
  using CreateAttributes = AdapterHostKey;
};

SAI_ATTRIBUTE_NAME(TamEvent, Type)
SAI_ATTRIBUTE_NAME(TamEvent, ActionList)
SAI_ATTRIBUTE_NAME(TamEvent, CollectorList)
SAI_ATTRIBUTE_NAME(TamEvent, SwitchEventType)

struct SaiTamTraits {
  static constexpr sai_object_type_t ObjectType = SAI_OBJECT_TYPE_TAM;
  using SaiApiT = TamApi;
  using EnumType = sai_tam_attr_t;
  struct Attributes {
    using EventObjectList = SaiAttribute<
        EnumType,
        SAI_TAM_ATTR_EVENT_OBJECTS_LIST,
        std::vector<sai_object_id_t>>;
    using TamBindPointList = SaiAttribute<
        EnumType,
        SAI_TAM_ATTR_TAM_BIND_POINT_TYPE_LIST,
        std::vector<sai_int32_t>>;
  };
  using AdapterKey = TamSaiId;
  using AdapterHostKey =
      std::tuple<Attributes::EventObjectList, Attributes::TamBindPointList>;
  using CreateAttributes = AdapterHostKey;
};

SAI_ATTRIBUTE_NAME(Tam, EventObjectList)
SAI_ATTRIBUTE_NAME(Tam, TamBindPointList)

class TamApi : public SaiApi<TamApi> {
 public:
  static constexpr sai_api_t ApiType = SAI_API_TAM;
  TamApi() {
    sai_status_t status =
        sai_api_query(ApiType, reinterpret_cast<void**>(&api_));
    saiApiCheckError(status, ApiType, "Failed to query for port api");
  }

 private:
  // TAM
  sai_status_t _create(
      TamSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_tam(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(TamSaiId id) const {
    return api_->remove_tam(id);
  }

  sai_status_t _getAttribute(TamSaiId id, sai_attribute_t* attr) const {
    return api_->get_tam_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(TamSaiId id, const sai_attribute_t* attr) const {
    return api_->set_tam_attribute(id, attr);
  }

  // TAM Event
  sai_status_t _create(
      TamEventSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_tam_event(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(TamEventSaiId id) const {
    return api_->remove_tam_event(id);
  }

  sai_status_t _getAttribute(TamEventSaiId id, sai_attribute_t* attr) const {
    return api_->get_tam_event_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(TamEventSaiId id, const sai_attribute_t* attr)
      const {
    return api_->set_tam_event_attribute(id, attr);
  }

  // TAM Event Action
  sai_status_t _create(
      TamEventActionSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_tam_event_action(
        rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(TamEventActionSaiId id) const {
    return api_->remove_tam_event_action(id);
  }

  sai_status_t _getAttribute(TamEventActionSaiId id, sai_attribute_t* attr)
      const {
    return api_->get_tam_event_action_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(
      TamEventActionSaiId id,
      const sai_attribute_t* attr) const {
    return api_->set_tam_event_action_attribute(id, attr);
  }

  // TAM Report
  sai_status_t _create(
      TamReportSaiId* id,
      sai_object_id_t switch_id,
      size_t count,
      sai_attribute_t* attr_list) const {
    return api_->create_tam_report(rawSaiId(id), switch_id, count, attr_list);
  }

  sai_status_t _remove(TamReportSaiId id) const {
    return api_->remove_tam_report(id);
  }

  sai_status_t _getAttribute(TamReportSaiId id, sai_attribute_t* attr) const {
    return api_->get_tam_report_attribute(id, 1, attr);
  }

  sai_status_t _setAttribute(TamReportSaiId id, sai_attribute_t* attr) const {
    return api_->set_tam_report_attribute(id, attr);
  }

  sai_tam_api_t* api_;
  friend class SaiApi<TamApi>;
};

} // namespace facebook::fboss
