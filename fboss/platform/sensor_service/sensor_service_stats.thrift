namespace cpp2 facebook.fboss.stats
namespace go neteng.fboss.sensor_service_stats
namespace py neteng.fboss.sensor_service_stats
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.sensor_service_stats

include "fboss/platform/sensor_service/if/sensor_service.thrift"
include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

struct SensorServiceStats {
  // Map of sensor name to SensorData
  1: map<string, sensor_service.SensorData> sensorData;
}
