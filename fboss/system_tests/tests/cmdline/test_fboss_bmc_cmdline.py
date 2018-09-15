from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import re

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags
from fboss.system_tests.testutils.cmdline_tests_helper import (
    check_binary_exist, check_subprocess_output)


@test_tags("cmdline", "new-test")
class FbossBmcCmdline(FbossBaseSystemTest):

    FBOSS_CMD = '/usr/local/bin/fboss'
    MIN_C = 4.5
    MAX_C = 60.0
    MIN_F = 40.0
    MAX_F = 140.0
    TEMP_PATTERN = re.compile('^.+ Temp: (\+|-)[0-9.]+ (C|F)$')
    FAN_PATTERN = re.compile('^Fan [0-6] (front|rear): [0-9]+ RPM$')
    POWER_PATTERN = re.compile('^Power Supply [0-9]$')
    POWER_NAME_PATTERN = re.compile('^Name: [0-9a-z-]+$')
    POWER_IO_PATTERN = re.compile('^(Input|Output) [a-zA-Z]+: ')

    def _check_temp(self, line_split, temp_location):
        '''
            Accepts an array of strings and an index in the array
            indicating the location of "Temp:"
            Asserts that next two elements afer temp_location are
            * +/-XX.X = valid float representing temperature
            * C/F = temperature unit (C or F)

            Returns temperature (float) and tempeture unit (string)

        '''
        try:
            temperature_str = line_split[temp_location + 1]
            temperature = float(temperature_str)
        except IndexError:
            self.fail(msg="Missing Temparture in: " + line_split)
        except ValueError:
            self.fail(msg="Could not convert temperature to float in " +
             temperature_str)
        except Exception:
            self.fail(msg="Temperature conversion failed in " + line_split)

        try:
            temperature_unit = str(line_split[temp_location + 2])
        except IndexError:
            self.fail(msg="Missing temperature unit in:  " + line_split)
        except Exception:
            self.fail(msg="Could not load temperature unit in " + line_split)

        self.assertTrue(temperature_unit in ["F", "C"],
            msg="Temperature Unit must be C or F")

        return temperature, temperature_unit

    def _check_temp_range(self, temperature, temperature_unit):
        '''
            Checks if temperature is within pre-specified range:
            - 40F and 140F
            - 4.5C and 60C
        '''
        self.assertGreaterEqual(temperature,
            FbossBmcCmdline.MIN_F if temperature_unit == "F"
            else FbossBmcCmdline.MIN_C,
            msg="Temperature below minimum")
        self.assertLessEqual(temperature,
            FbossBmcCmdline.MAX_F if temperature_unit == "F"
            else FbossBmcCmdline.MAX_C,
            msg="Temperature above maximum")

    def test_fboss_bmc_temp_cmd(self):
        """
        Test the 'fboss bmc temp' command

        Expected output

        $ fboss -H rsw1ap.20.snc1 bmc temp
        Inlet Temp: +23.6 C
        Switch Temp: +27.6 C
        Outlet Temp: +28.4 C
        CPU Temp: +24.1 C
        DIMM0 Temp: +30.8 C

        All temperatures should be populated with reasonable values -
        between 40F and 140F (4.5C and 60C)
        """

        # Does the binary exist?
        self.assertTrue(check_binary_exist(FbossBmcCmdline.FBOSS_CMD),
            msg="fboss command not found")

        cmd = [FbossBmcCmdline.FBOSS_CMD,
                '-H', self.test_topology.switch.name,
                'bmc',
                'temp'
               ]

        try:
            output = check_subprocess_output(cmd).splitlines()
        except Exception as e:
            self.fail(e)

        # as we step through each line:
        # - verify that each line follows expected output
        # - verify that each line contains a temperature
        # - verify that temperature unit is F or C
        # - verify that temperature is within range

        for line in output:
            self.assertIsNotNone(FbossBmcCmdline.TEMP_PATTERN.match(line),
                msg="Line doesn't match the expected output: " + line)
            line_split = line.split()
            temp_location = line_split.index("Temp:")
            temperature, temperature_unit = self._check_temp(line_split,
                temp_location)
            self._check_temp_range(temperature, temperature_unit)

    def test_fboss_bmc_fan_cmd(self):
        """
        Tests the 'fboss bmc fan' command

        Note to the future: system_tests currently don't support galaxy devices,
        but when we do, know that galaxy FC/LC cards have 0 fan

        Expected output:

        $ fboss -H rsw1ab.20.snc1 bmc fan
        Fan 1 front: 9750 RPM
        Fan 1 rear: 6900 RPM
        Fan 2 front: 9900 RPM
        Fan 2 rear: 6900 RPM
        Fan 3 front: 9900 RPM
        Fan 3 rear: 6900 RPM
        Fan 4 front: 9900 RPM
        Fan 4 rear: 6750 RPM
        Fan 5 front: 9750 RPM
        Fan 5 rear: 6600 RPM

        We expect there to be four or five fans labeled 0+ or 1+ with both front
        and rear present in the output. This test will check that there are
        between 1 and 12 lines of code inclusive, and that each line matches the
        regex.

        """

        # Does the binary exist?
        self.assertTrue(check_binary_exist(FbossBmcCmdline.FBOSS_CMD),
            msg="fboss command not found")

        cmd = [FbossBmcCmdline.FBOSS_CMD,
                "-H", self.test_topology.switch.name,
                "bmc", "fan"
               ]

        try:
            output = check_subprocess_output(cmd).splitlines()
        except Exception as e:
            self.fail(e)

        self.assertTrue(len(output) > 0 and len(output) <= 12,
            msg="fboss bmc fan command produced unexpected output size")

        for line in output:
            self.assertTrue(FbossBmcCmdline.FAN_PATTERN.match(line) is not None,
                msg="fboss bmc fan command output did not match regex")

    def test_fboss_bmc_power_cmd(self):
        """
        Tests the 'fboss bmc power' command

        Expected output:

        $ fboss -H rsw1at.20.snc1 bmc power

        Power Supply 1
        Name: pfe1100-i2c-7-59
        Input Voltage: +208.50 V
        Output Current: +0.00 A
        Output Power: 54.00 W
        Output Voltage: +12.01 V

        Power Supply 2
        Name: pfe1100-i2c-7-5a
        Input Voltage: +212.00 V
        Output Current: +2.50 A
        Output Power: 30.00 W
        Output Voltage: +12.05 V

        We expect there to be one or two power supply labeled 1, 2,
        each with a name and several input/output parameters.
        This test will check that each line matches the corresponding regex.

        """

        # Does the binary exist?
        self.assertTrue(check_binary_exist(FbossBmcCmdline.FBOSS_CMD),
            msg="fboss command not found")

        cmd = [FbossBmcCmdline.FBOSS_CMD,
                "-H", self.test_topology.switch.name,
                "bmc", "power"
               ]

        try:
            output = check_subprocess_output(cmd).splitlines()
        except Exception as e:
            self.fail(e)

        for line in output:
            if len(line) == 0:
                continue
            self.assertTrue(
                FbossBmcCmdline.POWER_PATTERN.match(line) is not None
                or FbossBmcCmdline.POWER_NAME_PATTERN.match(line) is not None
                or FbossBmcCmdline.POWER_IO_PATTERN.match(line) is not None,
                msg="fboss bmc power command output did not match regex")
