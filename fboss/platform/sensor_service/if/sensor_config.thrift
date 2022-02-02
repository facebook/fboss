namespace cpp2 facebook.fboss.platform.sensor_config
namespace go neteng.fboss.platform.sensor_config
namespace py neteng.fboss.platform.sensor_config
namespace py3 neteng.fboss.platform.sensor_config
namespace py.asyncio neteng.fboss.platform.asyncio.sensor_config

struct Sensor {
  /* Either sysfs path or the path in sensors JSON output */
  1: string path;
  /* Max threshold; optional */
  2: float maxVal = 65535.0;
  /* Min threshold; optional */
  3: float minVal = -65535.0;
  /* Targeted Value; optional */
  4: float targetVal;
  /* Compute method, e.g. @*0.1; optional */
  5: string compute;
  /* unit for sensor value, e.g. V, A, RPM, etc. */
  6: string unit;
}

/* Name -> sensor map, the name will be in this format: "SUB_FRU_1:SUB_FRU_2:...:SUB_FRU_N:SENSOR_NAME" */
typedef map<string, Sensor> sensorMap

/**
* The configuraiton for sensor mapping, each platform should provide one from
* vendor or derive from vendor spec sheet
*/
struct SensorConfig {
  /* source can be "lmsensor", "sysfs", or "mock" */
  1: string source;
  /* map of "FRU" -> SensorMap */
  2: map<string, sensorMap> sensorMapList;
}
