// Copyright 2021- Facebook. All rights reserved.

// Implementation of OdsStreamer class. Refer to .h
// for functional description
#include "fboss/platform/fan_service/OdsStreamer.h"

#include "fboss/platform/fan_service/SensorData.h"

// FB service routines for streaming ODS data
#include "common/network/NetworkUtil.h"
#include "maestro/if/OdsRouter/gen-cpp2/OdsRouter.h"
#include "monitoring/common/OdsCategoryId.h"
#include "servicerouter/client/cpp2/ServiceRouter.h"

// Folly library header file for conversion and log
#include <folly/Conv.h>
#include <folly/logging/xlog.h>
namespace {

int publishToOds(
    folly::EventBase* evb,
    std::vector<facebook::maestro::ODSAppValue> values,
    std::string odsTier) {
  int rc = 0;
  XLOG(INFO) << "ODS Streamer : Publisher : Entered. ";
  try {
    facebook::maestro::SetOdsRawValuesRequest request;
    *request.dataPoints_ref() = std::move(values);
    /* logging counters of type double */
    /* prepare the client */
    auto odsClient = folly::to_shared_ptr(
        facebook::servicerouter::cpp2::getClientFactory()
            .getSRClientUnique<facebook::maestro::OdsRouterAsyncClient>(
                odsTier, evb));
    /* log data points to ODS */
    odsClient->sync_setOdsRawValues(request);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error while publishing to ODS : " << folly::exceptionStr(ex);
    rc = -1;
  }
  XLOG(INFO) << "ODS Streamer : Publisher : Exiting with rc=" << rc;
  return rc;
}

facebook::maestro::ODSAppValue
getOdsAppValue(std::string key, int64_t value, uint64_t timeStampSec) {
  facebook::maestro::ODSAppValue retVal;
  retVal.key_ref() = key;
  retVal.value_ref() = value;
  retVal.unixTime_ref() = static_cast<int64_t>(timeStampSec);
  retVal.entity_ref() = facebook::network::NetworkUtil::getLocalHost(true);
  retVal.category_id_ref() = folly::to_signed(
      static_cast<int32_t>(facebook::monitoring::OdsCategoryId::ODS_FBOSS));
  return retVal;
}
} // namespace

namespace facebook::fboss::platform {

int OdsStreamer::postData(folly::EventBase* evb, const SensorData& sensorData)
    const {
  int rc = 0;
  std::vector<facebook::maestro::ODSAppValue> entriesToStream;
  float value = 0;
  int64_t int64Value;
  XLOG(INFO) << "ODS Streamer : Started";
  for (const auto& [sensorName, entry] : sensorData) {
    switch (entry.sensorEntryType) {
      case SensorEntryType::kSensorEntryInt:
        value = std::get<int>(entry.value);
        break;
      case SensorEntryType::kSensorEntryFloat:
        value = std::get<float>(entry.value);
    }
    int64Value = static_cast<int64_t>(value * 1000.0);
    XLOG(INFO) << "ODS Streamer : Packing " << entry.name << " : " << value;
    entriesToStream.push_back(getOdsAppValue(
        entry.name, value, sensorData.getLastUpdated(entry.name)));
  }
  XLOG(INFO) << "ODS Streamer : Data packed. Publishing";
  rc = publishToOds(evb, entriesToStream, odsTier_);
  XLOG(INFO) << "ODS Streamer : Done publishing with rc : " << rc;
  return rc;
}
} // namespace facebook::fboss::platform
