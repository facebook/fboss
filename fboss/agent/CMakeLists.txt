cmake_minimum_required(VERSION 3.0)

set(R /home/aalexandre/local/fbcode.cpp)

set(CMAKE_CXX_COMPILER
  "/home/engshare/third-party2/gcc/4.8.1/centos6-native/cc6c9dc/bin/g++")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -I../../.."add_compile_options)
add_compile_options(-std=gnu++11)
add_compile_options(-I${R})
add_compile_options(-I${R}/third-party2/glog/0.3.2/gcc-4.8.1-glibc-2.17/6992e57/include)
add_compile_options(-I${R}/third-party2/gflags/2.1.1/gcc-4.8.1-glibc-2.17/c3f970a/include)
add_compile_options(-I${R}/third-party2/double_conversion/20141126/gcc-4.8.1-glibc-2.17/c3f970a/include)
add_compile_options(-I${R}/_bin/configerator/distribution/if/4f96419/standalone-cpp.header-tree)
add_compile_options(-I${R}/third-party2/libevent/default/gcc-4.8.1-glibc-2.17/4cb9699/include/)
add_compile_options(-I${R}/_bin/common/fb303/if/4f96419/fb303-cpp.header-tree)
add_compile_options(-I${R}/_bin/fboss/agent/if/f0dc9eb/ctrl-cpp2.header-tree)
add_compile_options(-I${R}/_bin/neteng/fboss/if/f0dc9eb/fboss-cpp2.header-tree)
add_compile_options(-I${R}/_bin/common/fb303/if/f0dc9eb/fb303-cpp2.header-tree)
add_compile_options(-I${R}/_bin/common/network/if/f0dc9eb/if-cpp2.header-tree)
add_compile_options(-I${R}/_bin/configerator/distribution/if/4f96419/standalone-cpp.header-tree)
add_compile_options(-I${R}/_bin/common/network/if/4f96419/if-cpp.header-tree)
add_compile_options(-I${R}/_bin/fboss/agent/if/f0dc9eb/optic-cpp2.header-tree)

# For Utils
add_compile_options(-I${R}/_bin/configerator/structs/neteng/fboss/4f96419/fboss-cpp.header-tree)

# For core
add_compile_options(-I${R}/third-party2/iproute2/v3.12.0/gcc-4.8.1-glibc-2.17/c3f970a/include)

add_compile_options(-DFOLLY_HAVE_DEPRECATED_ASSOC=0)

project("ctrl")

# boost
set(BOOST_ROOT ${R}/third-party2/boost/1.57.0/centos6-native/24f9910/include)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(src
  Main.cpp
)

add_library(main
  Main.cpp
)

add_library(utils
  Utils.cpp
)

add_library(core
  ApplyThriftConfig.cpp
  ArpHandler.cpp
  DHCPv4Handler.cpp
  DHCPv6Handler.cpp
  IPv4Handler.cpp
  IPv6Handler.cpp
  IPHeaderV4.cpp
  HwSwitch.cpp
  PortStats.cpp
  SwitchStats.cpp
  SwSwitch.cpp
  TunIntf.cpp
  TunManager.cpp
  UDPHeader.cpp
  ndp/IPv6RouteAdvertiser.cpp
  SfpModule.cpp
  SfpMap.cpp
  LldpManager.cpp
  Platform.cpp
  NeighborUpdater.cpp
)
