#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

from fboss.cli.commands import commands as cmds


class ProductInfoCmd(cmds.FbossCmd):
    def run(self, detail):
        with self._create_agent_client() as client:
            resp = client.getProductInfo()
        if detail:
            self._print_product_details(resp)
        else:
            self._print_product_info(resp)

    def _print_product_info(self, productInfo):
        print("Product: %s" % (productInfo.product))
        print("OEM: %s" % (productInfo.oem))
        print("Serial: %s" % (productInfo.serial))

    def _print_product_details(self, productInfo):
        print("Product: %s" % (productInfo.product))
        print("OEM: %s" % (productInfo.oem))
        print("Serial: %s" % (productInfo.serial))
        print("Management MAC Address: %s" % (productInfo.mgmtMac))
        print("BMC MAC Address: %s" % (productInfo.bmcMac))
        print("Extended MAC Start: %s" % (productInfo.macRangeStart))
        print("Extended MAC Size: %s" % (productInfo.macRangeSize))
        print("Assembled At: %s" % (productInfo.assembledAt))
        print("Product Asset Tag: %s" % (productInfo.assetTag))
        print("Product Part Number: %s" % (productInfo.partNumber))
        print("Product Production State: %s" % (productInfo.productionState))
        print("Product Sub-Version: %s" % (productInfo.subVersion))
        print("Product Version: %s" % (productInfo.productVersion))
        print("System Assembly Part Number: %s" % (productInfo.systemPartNumber))
        print("System Manufacturing Date: %s" % (productInfo.mfgDate))
        print("PCB Manufacturer: %s" % (productInfo.pcbManufacturer))
        print("Facebook PCBA Part Number: %s" % (productInfo.fbPcbaPartNumber))
        print("Facebook PCB Part Number: %s" % (productInfo.fbPcbPartNumber))
        print("ODM PCBA Part Number: %s" % (productInfo.odmPcbaPartNumber))
        print("ODM PCBA Serial Number: %s" % (productInfo.odmPcbaSerial))
        print("Version: %s" % (productInfo.version))
