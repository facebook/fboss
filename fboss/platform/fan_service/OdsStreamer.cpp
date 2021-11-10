// Copyright 2021- Facebook. All rights reserved.

// Implementation of OdsStreamer class. Refer to .h
// for functional description
#include "fboss/platform/fan_service/OdsStreamer.h"

DECLARE_string(ods_tier);
namespace facebook::fboss::platform {

OdsStreamer::OdsStreamer(std::shared_ptr<SensorData> pSd, std::string oT) {
  pSensorData_ = pSd;
  odsTier_ = oT;
}

OdsStreamer::~OdsStreamer() {}

int OdsStreamer::publishToOds(
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

facebook::maestro::ODSAppValue OdsStreamer::getOdsAppValue(
    std::string key,
    int64_t value,
    uint64_t timeStampSec) {
  facebook::maestro::ODSAppValue retVal;
  retVal.key_ref() = key;
  retVal.value_ref() = value;
  retVal.unixTime_ref() = (int64_t)timeStampSec;
  retVal.entity_ref() = facebook::network::NetworkUtil::getLocalHost(true);
  retVal.category_id_ref() =
      folly::to_signed(int32_t(facebook::monitoring::OdsCategoryId::ODS_FBOSS));
  return retVal;
}

int OdsStreamer::postData(folly::EventBase* evb) {
  int rc = 0;
  std::vector<facebook::maestro::ODSAppValue> dataToStream;
  float value = 0;
  int64_t int64Value;
  XLOG(INFO) << "ODS Streamer : Started";
  std::vector<std::string> keyList = pSensorData_->getKeyLists();
  for (auto sensorName = keyList.begin(); sensorName != keyList.end();
       ++sensorName) {
    if (pSensorData_->getSensorEntryType(*sensorName) == kSensorEntryInt) {
      value = (float)pSensorData_->getSensorDataInt(*sensorName);
    } else {
      value = pSensorData_->getSensorDataFloat(*sensorName);
    }
    int64Value = static_cast<int64_t>(value * 1000.0);
    XLOG(INFO) << "ODS Streamer : Packing " << *sensorName << " : " << value;
    dataToStream.push_back(getOdsAppValue(
        *sensorName,
        value,
        (uint64_t)pSensorData_->getLastUpdated(*sensorName)));
  }
  XLOG(INFO) << "ODS Streamer : Data packed. Publishing";
  rc = publishToOds(evb, dataToStream, odsTier_);
  XLOG(INFO) << "ODS Streamer : Done publishing with rc : " << rc;
  return rc;
}
} // namespace facebook::fboss::platform
