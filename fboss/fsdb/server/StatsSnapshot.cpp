// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/server/StatsSnapshot.h"
#include <folly/String.h>

namespace facebook::fboss::fsdb {

StatsSnapshot::StatsSnapshot(const FqMetric& fqMetric)
    : fqMetric_(fqMetric), timestampSec_(Utils::getNowSec()) {}

StatsSnapshot::StatsSnapshot(
    const FqMetric& fqMetric,
    const int64_t timestampSec,
    const std::string& value)
    : fqMetric_(fqMetric),
      timestampSec_(timestampSec),
      payload_(fromValue(value)) {}

StatsSnapshot::StatsSnapshot(const FqMetric& fqMetric, bool publisherConnected)
    : StatsSnapshot(fqMetric) {
  // TODO: use separate ctors for created and destroyed control pkt snapshots
  if (publisherConnected) {
    payload_.set_available();
    // TODO: set schema to inform subscriber
  } else {
    payload_.set_unavailable();
  }
}

StatsSnapshot::StatsSnapshot(const FqMetric& fqMetric, Vectors&& vectors)
    : StatsSnapshot(fqMetric) {
  payload_.set_dataCompressed();
  payload_.dataCompressed_ref()->vectors() = vectors;
}

// NOTE: omit publisherId as part of key as rocksdb is sharded by publisher
std::string StatsSnapshot::key() const {
  return folly::to<std::string>(
      *fqMetric_.metric(), ".", Utils::stringifyTimestampSec(timestampSec()));
}

std::string StatsSnapshot::value() const {
  CHECK_NE(payload_.getType(), StatsSubUnitPayload::Type::data);

  folly::IOBufQueue q;
  apache::thrift::BinaryProtocolWriter writer;
  writer.setOutput(&q);

  payload_.write(&writer);

  std::string out;
  q.appendToString(out);
  return out;
}

// static
StatsSubUnitPayload StatsSnapshot::fromValue(const std::string& value) {
  auto buf = folly::IOBuf::copyBuffer(value);
  apache::thrift::BinaryProtocolReader reader;
  reader.setInput(buf.get());

  StatsSubUnitPayload payload;
  payload.read(&reader);

  CHECK_NE(payload.getType(), StatsSubUnitPayload::Type::data);
  return payload;
}

bool StatsSnapshot::operator==(const StatsSnapshot& other) const {
  return (fqMetric_ == other.fqMetric_) &&
      (timestampSec_ == other.timestampSec_) &&
      ThriftVisitationEquals(payload_, other.payload_);
}

const Vectors& StatsSnapshot::getVectors() const {
  CHECK_EQ(payload_.getType(), StatsSubUnitPayload::Type::dataCompressed);
  return *payload_.dataCompressed_ref()->vectors();
}

void StatsSnapshot::decompress() {
  if (payload_.getType() != StatsSubUnitPayload::Type::dataCompressed) {
    return;
  }

  Counters counters;
  for (const auto& [exportTypeStr, v] :
       *payload_.dataCompressed_ref()->vectors()) {
    auto i = 0;
    for (const auto& durationSuffix : Utils::kDurationSuffixes) {
      counters.insert(
          {folly::to<std::string>(
               *fqMetric_.metric(), ".", exportTypeStr, durationSuffix),
           v.at(i++)});
    }
  }

  payload_.set_data();
  payload_.data_ref()->counters() = std::move(counters);
}

} // namespace facebook::fboss::fsdb
