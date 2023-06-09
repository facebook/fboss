# Make to build libraries and binaries in fboss/platform/platform_manager

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  platform_manager_snapshot_cpp2
  fboss/platform/platform_manager/platform_manager_snapshot.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  platform_manager_presence_cpp2
  fboss/platform/platform_manager/platform_manager_presence.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  platform_manager_config_cpp2
  fboss/platform/platform_manager/platform_manager_config.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    platform_manager_presence_cpp2
)

add_fbthrift_cpp_library(
  platform_manager_service_cpp2
  fboss/platform/platform_manager/platform_manager_service.thrift
  SERVICES
    PlatformManagerService
  OPTIONS
    json
    reflection
  DEPENDS
   platform_manager_snapshot_cpp2
)

add_executable(platform_manager
  fboss/platform/platform_manager/Main.cpp
  fboss/platform/platform_manager/PlatformExplorer.cpp
  fboss/platform/platform_manager/PlatformI2cExplorer.cpp
  fboss/platform/platform_manager/PlatformValidator.cpp
  fboss/platform/platform_manager/PlatformManagerHandler.cpp
)

target_link_libraries(platform_manager
  fb303::fb303
  platform_config_lib
  platform_utils
  platform_manager_config_cpp2
  platform_manager_presence_cpp2
  platform_manager_service_cpp2
  platform_manager_snapshot_cpp2
)

install(TARGETS platform_manager)
