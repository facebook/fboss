# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.16)

project(FbossExportedLibraries VERSION 1.0.0 LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

find_package(Boost REQUIRED COMPONENTS container)
find_package(Glog REQUIRED)
find_package(folly CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
# FBThrift's exported targets reference wangle::wangle, whose exported targets
# reference fizz::fizz. Their package configs do not load these dependencies.
find_package(fizz CONFIG REQUIRED)
find_package(wangle CONFIG REQUIRED)
find_package(FBThrift CONFIG REQUIRED)

get_filename_component(
  FBOSS_SOURCE_ROOT
  "${CMAKE_CURRENT_LIST_DIR}/../.."
  ABSOLUTE
)

add_library(nodebase
  ${FBOSS_SOURCE_ROOT}/fboss/agent/StrongTypes.h
  ${FBOSS_SOURCE_ROOT}/fboss/agent/state/NodeBase.cpp
  ${FBOSS_SOURCE_ROOT}/fboss/agent/state/NodeBase.h
  ${FBOSS_SOURCE_ROOT}/fboss/agent/state/NodeBase-defs.h
)
target_compile_features(nodebase PUBLIC cxx_std_20)
target_include_directories(nodebase
  PUBLIC
    $<BUILD_INTERFACE:${FBOSS_SOURCE_ROOT}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(nodebase
  PUBLIC
    Boost::container
    Folly::folly
    fmt::fmt
    glog::glog
)
set_target_properties(nodebase PROPERTIES EXPORT_NAME NodeBase)
add_library(FBOSS::NodeBase ALIAS nodebase)

add_library(fboss_radix_tree INTERFACE)
target_compile_features(fboss_radix_tree INTERFACE cxx_std_20)
target_include_directories(fboss_radix_tree
  INTERFACE
    $<BUILD_INTERFACE:${FBOSS_SOURCE_ROOT}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(fboss_radix_tree
  INTERFACE
    Folly::folly
    glog::glog
)
set_target_properties(fboss_radix_tree PROPERTIES EXPORT_NAME RadixTree)
add_library(FBOSS::RadixTree ALIAS fboss_radix_tree)

add_library(fboss_exponential_backoff
  ${FBOSS_SOURCE_ROOT}/fboss/lib/ExponentialBackoff.cpp
  ${FBOSS_SOURCE_ROOT}/fboss/lib/ExponentialBackoff.h
)
target_compile_features(fboss_exponential_backoff PUBLIC cxx_std_20)
target_include_directories(fboss_exponential_backoff
  PUBLIC
    $<BUILD_INTERFACE:${FBOSS_SOURCE_ROOT}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(fboss_exponential_backoff PRIVATE glog::glog)
set_target_properties(
  fboss_exponential_backoff
  PROPERTIES EXPORT_NAME ExponentialBackoff
)
add_library(FBOSS::ExponentialBackoff ALIAS fboss_exponential_backoff)

add_library(fboss_log_thrift_call
  ${FBOSS_SOURCE_ROOT}/fboss/lib/LogThriftCall.cpp
  ${FBOSS_SOURCE_ROOT}/fboss/lib/LogThriftCall.h
  ${FBOSS_SOURCE_ROOT}/fboss/lib/ThriftCallDurationLogger.h
  ${FBOSS_SOURCE_ROOT}/fboss/lib/oss/LogThriftCall.cpp
)
target_compile_features(fboss_log_thrift_call PUBLIC cxx_std_20)
target_include_directories(fboss_log_thrift_call
  PUBLIC
    $<BUILD_INTERFACE:${FBOSS_SOURCE_ROOT}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(fboss_log_thrift_call
  PUBLIC
    FBThrift::thriftcpp2
    Folly::folly
)
set_target_properties(
  fboss_log_thrift_call
  PROPERTIES EXPORT_NAME LogThriftCall
)
add_library(FBOSS::LogThriftCall ALIAS fboss_log_thrift_call)

add_library(fboss_alert_logger
  ${FBOSS_SOURCE_ROOT}/fboss/lib/AlertLogger.cpp
  ${FBOSS_SOURCE_ROOT}/fboss/lib/AlertLogger.h
)
target_compile_features(fboss_alert_logger PUBLIC cxx_std_20)
target_include_directories(fboss_alert_logger
  PUBLIC
    $<BUILD_INTERFACE:${FBOSS_SOURCE_ROOT}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(fboss_alert_logger
  PUBLIC
    Folly::folly
    fmt::fmt
)
set_target_properties(fboss_alert_logger PROPERTIES EXPORT_NAME AlertLogger)
add_library(FBOSS::AlertLogger ALIAS fboss_alert_logger)

# This aggregate target defines the FBOSS libraries supported by standalone
# consumers such as BGP OSS. Keep this list limited to libraries that do not
# require the full FBOSS agent dependency graph.
add_library(fboss_exported_libraries INTERFACE)
target_link_libraries(fboss_exported_libraries
  INTERFACE
    FBOSS::AlertLogger
    FBOSS::ExponentialBackoff
    FBOSS::LogThriftCall
    FBOSS::NodeBase
    FBOSS::RadixTree
)
set_target_properties(
  fboss_exported_libraries
  PROPERTIES EXPORT_NAME ExportedLibraries
)
add_library(FBOSS::ExportedLibraries ALIAS fboss_exported_libraries)

install(
  TARGETS
    fboss_alert_logger
    fboss_exported_libraries
    fboss_exponential_backoff
    fboss_log_thrift_call
    nodebase
    fboss_radix_tree
  EXPORT FbossExportedLibrariesTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
  FILES ${FBOSS_SOURCE_ROOT}/fboss/agent/StrongTypes.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fboss/agent
)
install(
  FILES
    ${FBOSS_SOURCE_ROOT}/fboss/agent/state/NodeBase-defs.h
    ${FBOSS_SOURCE_ROOT}/fboss/agent/state/NodeBase.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fboss/agent/state
)
install(
  FILES
    ${FBOSS_SOURCE_ROOT}/fboss/lib/AlertLogger.h
    ${FBOSS_SOURCE_ROOT}/fboss/lib/ExponentialBackoff.h
    ${FBOSS_SOURCE_ROOT}/fboss/lib/LogThriftCall.h
    ${FBOSS_SOURCE_ROOT}/fboss/lib/RadixTree-inl.h
    ${FBOSS_SOURCE_ROOT}/fboss/lib/RadixTree.h
    ${FBOSS_SOURCE_ROOT}/fboss/lib/ThriftCallDurationLogger.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fboss/lib
)

set(FBOSS_EXPORTED_LIBRARIES_CMAKE_DIR
  ${CMAKE_INSTALL_LIBDIR}/cmake/FbossExportedLibraries
)
configure_package_config_file(
  ${CMAKE_CURRENT_LIST_DIR}/FbossExportedLibrariesConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/FbossExportedLibrariesConfig.cmake
  INSTALL_DESTINATION ${FBOSS_EXPORTED_LIBRARIES_CMAKE_DIR}
)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/FbossExportedLibrariesConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  EXPORT FbossExportedLibrariesTargets
  FILE FbossExportedLibrariesTargets.cmake
  NAMESPACE FBOSS::
  DESTINATION ${FBOSS_EXPORTED_LIBRARIES_CMAKE_DIR}
)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/FbossExportedLibrariesConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/FbossExportedLibrariesConfigVersion.cmake
  DESTINATION ${FBOSS_EXPORTED_LIBRARIES_CMAKE_DIR}
)
