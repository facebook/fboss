#!/usr/bin/env python3

from fboss_link_test_case import FbosslinkTestCase


class FbossLinkQsfpTests(FbosslinkTestCase):
    """
    Tests that focus on qsfp-service functionality.
    """

    def setUp(self) -> None:
        super().setUp()

    def test_fetch_all_DOM_data(self):
        qsfp_dom_info_map = self.get_all_qsfp_dom_values()
        dom_status = True
        for intf in qsfp_dom_info_map.keys():
            if not qsfp_dom_info_map[intf].present and intf not in [
                "eth2/1/1",
                "eth9/16/1",
            ]:
                self.log.error(f"Optic is not present on {intf}")
                dom_status = False
        self.assertTrue(dom_status, msg="one or more optic is not present")

    def test_qsfp_service_warmboot(self):
        self.log.info("Testing warm boot of qsfp service")
        portNames = self._get_all_interface_names()

        beforeOverallLinkFlapCount = self._getOverallLinkFlapCount()
        beforePortLinkFlapCounts = self._getPortsLinkFlapCount(portNames)

        # Restart qsfp service
        self.log.info("Restarting qsfp service in warmboot mode")
        self.warm_boot_qsfp_service()
        # Confirm that all transceivers are marked present
        self.check_transceiver_presence()
        # Confirm that the ports come back up
        self.check_port_status()
        # Make sure no flaps have occurred
        afterOverallLinkFlapCount = self._getOverallLinkFlapCount()
        afterPortLinkFlapCounts = self._getPortsLinkFlapCount(portNames)
        self._check_port_flap_counts(
            beforePortLinkFlapCounts, afterPortLinkFlapCounts, False
        )
        self.assertTrue(
            beforeOverallLinkFlapCount == afterOverallLinkFlapCount,
            msg="Unexpected flaps seen",
        )

    def test_qsfp_service_coldboot(self):
        self.log.info("Testing cold boot of qsfp service")
        portNames = self._get_all_interface_names()

        beforeOverallLinkFlapCount = self._getOverallLinkFlapCount()
        beforePortLinkFlapCounts = self._getPortsLinkFlapCount(portNames)

        # Restart qsfp service
        self.log.info("Restarting qsfp service in coldboot mode")
        self.cold_boot_qsfp_service()
        # Confirm that all transceivers are marked present
        self.check_transceiver_presence()
        # Confirm that the ports come back up
        self.check_port_status()
        # Make sure flaps have occurred to ensure cold boot indeed happened
        afterOverallLinkFlapCount = self._getOverallLinkFlapCount()
        afterPortLinkFlapCounts = self._getPortsLinkFlapCount(portNames)
        self._check_port_flap_counts(
            beforePortLinkFlapCounts, afterPortLinkFlapCounts, True
        )
        self.assertTrue(
            beforeOverallLinkFlapCount != afterOverallLinkFlapCount,
            msg="Unexpected flaps seen",
        )
