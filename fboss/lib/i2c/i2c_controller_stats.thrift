namespace cpp2 facebook.fboss
namespace py neteng.fboss.i2c_controller_stats

const i64 STAT_UNINITIALIZED = 0

struct I2cControllerStats {
  1: string controllerName_ = ""
  2: i64 readTotal_ = STAT_UNINITIALIZED
  3: i64 readFailed_ = STAT_UNINITIALIZED
  4: i64 readBytes_ = STAT_UNINITIALIZED
  5: i64 writeTotal_ = STAT_UNINITIALIZED
  6: i64 writeFailed_ = STAT_UNINITIALIZED
  7: i64 writeBytes_ = STAT_UNINITIALIZED
}
