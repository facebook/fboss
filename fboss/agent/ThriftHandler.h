/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <memory>
#include <string>
#include <vector>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "common/fb303/cpp/FacebookBase2.h"

namespace facebook { namespace fboss {

class SwSwitch;
class Vlan;

class ThriftHandler : virtual public FbossCtrlSvIf,
                      public fb303::FacebookBase2 {
 public:
  template<typename T>
  using ThriftCallback = std::unique_ptr<apache::thrift::HandlerCallback<T>>;
  typedef ThriftCallback<void> VoidCallback;

  typedef network::thrift::cpp2::Address Address;
  typedef network::thrift::cpp2::BinaryAddress BinaryAddress;
  typedef std::vector<Address> Addresses;
  typedef std::vector<BinaryAddress> BinaryAddresses;

  explicit ThriftHandler(SwSwitch* sw);

  fb303::cpp2::fb_status getStatus() override;
  void async_tm_getStatus(ThriftCallback<fb303::cpp2::fb_status> cb) override;

  void flushCountersNow() override;

  void addUnicastRoute(
      int16_t client, std::unique_ptr<UnicastRoute> route);
  void deleteUnicastRoute(
      int16_t client, std::unique_ptr<IpPrefix> prefix);
  void addUnicastRoutes(
      int16_t client, std::unique_ptr<std::vector<UnicastRoute>> routes);
  void deleteUnicastRoutes(
      int16_t client, std::unique_ptr<std::vector<IpPrefix>> prefixes);
  void syncFib(
      int16_t client, std::unique_ptr<std::vector<UnicastRoute>> routes);

  SwSwitch* getSw() const {
    return sw_;
  }

  void sendPkt(int32_t port, int32_t vlan,
      std::unique_ptr<folly::fbstring> data);
  void sendPktHex(int32_t port, int32_t vlan,
      std::unique_ptr<std::string> hex);

  int32_t flushNeighborEntry(std::unique_ptr<BinaryAddress> ip,
                             int32_t vlan) override;

  void getVlanAddresses(Addresses& addrs, int32_t vlan) override;
  void getVlanAddressesByName(Addresses& addrs,
      const std::unique_ptr<std::string> vlan) override;
  void getVlanBinaryAddresses(BinaryAddresses& addrs, int32_t vlan) override;
  void getVlanBinaryAddressesByName(BinaryAddresses& addrs,
      const std::unique_ptr<std::string> vlan) override;
  /* Returns the Ip Route for the address */
  void getIpRoute(UnicastRoute& route,
                  std::unique_ptr<Address> addr, int32_t vrfId) override;
  void getAllInterfaces(
      std::map<int32_t, InterfaceDetail>& interfaces) override;
  void getInterfaceList(std::vector<std::string>& interfaceList) override;
  void getRouteTable(std::vector<UnicastRoute>& routeTable) override;
  void getPortStatus(std::map<int32_t, PortStatus>& status,
                     std::unique_ptr<std::vector<int32_t>> ports) override;
  void getInterfaceDetail(InterfaceDetail& interfaceDetails,
                                          int32_t interfaceId) override;
  void getArpTable(std::vector<ArpEntryThrift>& arpTable) override;
  void getNdpTable(std::vector<NdpEntryThrift>& arpTable) override;

  /* Returns the SFP Dom information */
  void getSfpDomInfo(std::map<int32_t, SfpDom>& domInfos,
                     std::unique_ptr<std::vector<int32_t>> ports) override;

  BootType getBootType() override;

  void getLldpNeighbors(std::vector<LinkNeighborThrift>& results) override;

  void startPktCapture(std::unique_ptr<CaptureInfo> info);
  void stopPktCapture(std::unique_ptr<std::string> name);
  void stopAllPktCaptures();

 private:
  Vlan* getVlan(int32_t vlanId);
  Vlan* getVlan(const std::string& vlanName);
  template<typename ADDR_TYPE, typename ADDR_CONVERTER>
  void getVlanAddresses(const Vlan* vlan, std::vector<ADDR_TYPE>& addrs,
      ADDR_CONVERTER& converter);
  // Forbidden copy constructor and assignment operator
  ThriftHandler(ThriftHandler const &) = delete;
  ThriftHandler& operator=(ThriftHandler const &) = delete;

  void ensureConfigured(folly::StringPiece function);
  void ensureConfigured() {
    // This version of ensureConfigured() won't log
    ensureConfigured(folly::StringPiece(nullptr, nullptr));
  }
  /*
   * On a warm boot we need to prevent route updates
   * before a full FIB sync event. Otherwise if we get a
   * add and delete for a route that might lead us to believe
   * that the reference count for this route's egress object has
   * dropped to 0 but in reality we just haven't heard about all
   * the routes that may also point to this egress. This causes
   * errors when we try to delete the egress objects. t4155406
   * should fix this.
   */
  void ensureFibSynced(folly::StringPiece function);
  void ensureFibSynced() {
    // This version of ensureFibSynced() won't log
    ensureFibSynced(folly::StringPiece(nullptr, nullptr));
  }

  template<typename Result>
  void fail(const ThriftCallback<Result>& callback,
            const std::exception& ex) {
    FbossError error(folly::exceptionStr(ex));
    callback->exception(error);
  }

  /*
   * A pointer to the SwSwitch.  We don't own this.
   * It's the main program's responsibility to ensure that the SwSwitch exists
   * for the lifetime of the ThriftHandler.
   */
  SwSwitch* sw_;
};

}} // facebook::fboss
