# Make to build libraries and binaries in fboss/platform/reboot_cause_service

add_fbthrift_cpp_library(
  reboot_cause_config_cpp2
  fboss/platform/reboot_cause_service/if/reboot_cause_config.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  reboot_cause_service_cpp2
  fboss/platform/reboot_cause_service/if/reboot_cause_service.thrift
  SERVICES
    RebootCauseService
  OPTIONS
    json
    reflection
  DEPENDS
    fboss_cpp2
    reboot_cause_config_cpp2
)

add_library(reboot_cause_service_config_validator
  fboss/platform/reboot_cause_service/ConfigValidator.cpp
)

target_link_libraries(reboot_cause_service_config_validator
  fmt::fmt
  reboot_cause_config_cpp2
  Folly::folly
)
