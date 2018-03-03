from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os
import re
import subprocess

from fboss.system_tests.system_tests import FbossBaseSystemTest, test_tags


@test_tags("cmdline", "run-on-diff")
class FbossBmcCmdline(FbossBaseSystemTest):

    FBOSS_CMD = '/usr/local/bin/fboss'
    MIN_C = 4.5
    MAX_C = 60.0
    MIN_F = 40.0
    MAX_F = 140.0
    TEMP_PATTERN = re.compile('^.+ Temp: (\+|-)[0-9.]+ (C|F)$')

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

        # make sure binary exists
        self.assertTrue(os.path.exists(FbossBmcCmdline.FBOSS_CMD),
            msg="fboss command not found")
        cmd = [FbossBmcCmdline.FBOSS_CMD,
                '-H', self.test_topology.switch.name,
                'bmc',
                'temp'
               ]
        try:
            output = subprocess.check_output(cmd, encoding="utf-8")
        except Exception as e:
            self.fail(
                "Something went wrong when executing \
                fboss bmc temp command. \n {0}".format(e)
            )

        # make sure we got something
        self.assertTrue(output,
            msg="fboss bmc temp command produced no output")
        output = output.splitlines()

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
