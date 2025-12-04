package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct InterfaceErrorsModel {
  1: list<ErrorCounters> error_counters;
}

struct ErrorCounters {
  1: string interfaceName;
  2: i64 inputErrors;
  3: i64 inputDiscards;
  4: i64 outputErrors;
  5: i64 outputDiscards;
}
