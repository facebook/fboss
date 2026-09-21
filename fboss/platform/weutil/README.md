# weutil (Wedge Util for x86 and BMC) - A FBOSS Platform Utility For Reading EEPROMS

## Objective
1. The primary functionality of this utility is to read and parse all ID EEPROMs in FBOSS based network switches, then display the contents in a human readable format.

## Design Concept
* The weutil has all the logic that is needed for reading all EEPROMs on the Meta's network switch
* The utility supports Meta v1/2/3 and v4 EEPROM format
* The utility also supports parsing non-Meta format EEPROM (for select vendor platforms) into Meta format. In this case, some fields may not be populated, if the vendor EEPROM does not have such information.

## Arguments for x86 version only
* json : If set to true, weutil will return the output in JSON format, instead of plain text format
* eeprom : The "name" of the EEPROM you want to read.  If not specified, the mainboard EEPROM will be read (default value) (eg. weutil json=True eeprom=SCM)

## Arguments for BMC version only
* eeprom : The actual "device path" will be given as the first argument of BMC version of weutil. Specifies which eeprom to read. If not specified, the mainboard EEPROM will be read. (eg.  weutil /sys/bus/i2c/drivers/at24/6-0013/eeprom)

## Trouble Shooting
* The utility cannot read EEPROM without any argument / parameter given : This is likely the issue with BSP kernel module install or bad hardware, not the issue with weutil itself.
* The utility cannot read the EEPROM specified by the argument, while it can still read the default EEPROM : Please double check the HW path / name of the EEPROM you want to read and make sure there is no typo.
