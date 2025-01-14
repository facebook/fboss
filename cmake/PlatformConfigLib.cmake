# Make to build libraries and binaries in fboss/platform/config_lib

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake


file(GLOB_RECURSE required_configs 
  ${CMAKE_CURRENT_SOURCE_DIR}/fboss/platform/configs/*.json
)

set(generated_header 
  ${CMAKE_CURRENT_BINARY_DIR}/fboss/platform/config_lib/GeneratedConfig.h
)

get_filename_component(output_dir
  ${generated_header}
  DIRECTORY
)

add_custom_command(
  OUTPUT
    ${generated_header}
  COMMAND
    ${CMAKE_COMMAND} -E make_directory ${output_dir}
  COMMAND
    ${CMAKE_CURRENT_BINARY_DIR}/platform_config_lib_config_generator 
    --json_config_dir 
    ${CMAKE_CURRENT_SOURCE_DIR}/fboss/platform/configs 
    --install_dir 
    ${output_dir}
  DEPENDS
    ${required_configs}
    ${CMAKE_CURRENT_BINARY_DIR}/platform_config_lib_config_generator
)

add_executable(platform_config_lib_config_generator
  fboss/platform/config_lib/ConfigGenerator.cpp
)

target_link_libraries(platform_config_lib_config_generator
  led_manager_config_types_cpp2
  fan_service_config_validator
  fan_service_cpp2
  fw_util_config-cpp2-types
  platform_manager_config_validator
  platform_manager_config_cpp2
  platform_manager_presence_cpp2
  sensor_service_config_validator
  sensor_config_cpp2
  weutil_config_cpp2
  Folly::folly
)

add_executable(platform_config_lib_config_lib_test
  fboss/platform/config_lib/ConfigLibTest.cpp
)

target_link_libraries(platform_config_lib_config_lib_test
  platform_config_lib
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(platform_config_lib
  fboss/platform/config_lib/ConfigLib.cpp
  ${generated_header}
)

target_include_directories(platform_config_lib
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(platform_config_lib
  Folly::folly
  platform_name_lib
)
