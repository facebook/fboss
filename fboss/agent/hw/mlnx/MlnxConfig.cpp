/* 
 * Copyright (c) Mellanox Technologies. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "fboss/agent/hw/mlnx/MlnxConfig.h"

#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>

namespace facebook { namespace fboss {

void MlnxConfig::parseConfigXml(const std::string& pathToConfigFile) {
  using tokenizer = boost::tokenizer<boost::char_separator<char>>;
  using boost::property_tree::ptree;

  ptree pt;
  std::ifstream ifs(pathToConfigFile);

  read_xml(ifs, pt);

  boost::char_separator<char> sep{","};

  // traverse XML tree
  auto root = pt.get_child("root");
  for(const auto& v: root) {
    if (v.first == "device") {
      device.deviceId = v.second.get<uint8_t>("device-id");
      device.deviceMacAddress =
        v.second.get<std::string>("device-mac-address");
      for(const auto& vv: v.second.get_child("ports-list")) {
        if (vv.first == "port-info") {
          PortInfo portInfo;
          portInfo.localPort = vv.second.get<uint8_t>("local-port");
          portInfo.mappingMode = vv.second.get<uint8_t>("mapping-mode");
          // parse lanes array
          {
            auto LanesStr = vv.second.get<std::string>("lanes");
            tokenizer tokens(LanesStr, sep);
            BOOST_FOREACH(auto& token, tokens) {
              portInfo.lanes.push_back(std::stoi(token));
            }
          }
          portInfo.frontpanelPort = vv.second.get<uint8_t>("frontpanel-port");
          device.ports.push_back(portInfo);
        }
      }
    } else if (v.first == "router-resources") {
      // parse
      routerRsrc.maxVrfNum = v.second.get<uint32_t>("max-vrf", 0);
      routerRsrc.maxVlanRouterInterfaces =
        v.second.get<uint32_t>("max-vlan-router-interfaces", 0);
      routerRsrc.maxPortRouterInterfaces =
        v.second.get<uint32_t>("max-port-router-interface", 0);
      routerRsrc.maxRouterInterfaces =
        v.second.get<uint32_t>("max-router-interfaces", 0);
      routerRsrc.maxV4NeighEntries =
        v.second.get<uint32_t>("max-ipv4-neighbor-entires", 0);
      routerRsrc.maxV6NeighEntries =
        v.second.get<uint32_t>("max-ipv6-neighbor-entires", 0);
      routerRsrc.maxV4RouteEntries =
        v.second.get<uint32_t>("max-ipv4-route-entries", 0);
      routerRsrc.maxV6RouteEntries =
        v.second.get<uint32_t>("max-ipv6-route-entries", 0);
    }
  }
}

}} // facebook::fboss
