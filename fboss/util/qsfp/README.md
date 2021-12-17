FBOSS utility to enable Wedge400 optics
===========

The wedge400_qsfp_util python utility allows the active optical modules plugged
in the Wedge400 platform to be enabled/disabled. With this utility the optics
modules can be made to work on wedge400 platform using open source agent and
qsfp binary.
These optics will come out of reset and low power mode and work with default
configuration and settings. The 100G CWDM4 will work in 100G mode, the 200G
FR4 will work in 200G mode, the 400G FR4 optics will work in 400G mode. But
the non default speed configration will not work (example 400G FR4 optics in
200G mode) as that requires application selection inside the optics register
space which is not provided here.

Command Line
---------------

To enable all the optics modules on Wedge400 platform:
python3 ./wedge400_fpga_util.py --enable

To disable all the optics modules on Wedge400 platform:
python3 ./wedge400_fpga_util.py --disable

To get the Wedge400 module bitmap reset info
python3 ./wedge400_fpga_util.py --info
