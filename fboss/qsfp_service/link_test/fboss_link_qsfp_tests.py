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
