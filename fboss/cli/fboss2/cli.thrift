package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

/* These CLi option error codes have been defined to
  avoid using the catchall error code (1).
  These will also aid in proper unit testing.
  */
enum CliOptionResult {
  EOK = 0,
  KEY_ERROR = 1,
  VALUE_ERROR = 2,
  TYPE_ERROR = 3,
  OP_ERROR = 4,
  EXTRA_OPTIONS = 5,
  TERM_ERROR = 6,
  AGG_ARGUMENT_ERROR = 7,
}
