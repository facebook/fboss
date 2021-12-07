namespace cpp2 facebook.fboss.platform.sensor_config
namespace go neteng.fboss.platform.sensor_config
namespace py neteng.fboss.platform.sensor_config
namespace py3 neteng.fboss.platform.sensor_config
namespace py.asyncio neteng.fboss.platform.asyncio.sensor_config

struct Sensor {
  /* Either sysfs path or the path in sensors JSON output */
  1: string path;
  /* Meaningful name */
  2: string name;
  /* FRU name */
  3: string fru;
  4: string type;
  /* Overwrite data source, e.g. "lmsensor", "sysfs", "mock" */
  5: string sourceOverride;
  /* Max threshold */
  6: float maxVal = -65535.0;
  /* Min threshold */
  7: float minVal = -65535.0;
}

/**
* The configuraiton for sensor mapping, each platform should provide one from
* vendor or derive from vendor spec sheet
*/

struct SensorConfig {
  /* source can be "lmsensor", "sysfs", or "mock" */
  1: string source;
  2: map<string, Sensor> sensorMapList;
}
