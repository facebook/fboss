# Config Lib - Meta Implementation

## Objective
The primary objective of this module is to make it seamless to consume configs for platform binaries

## How it works
* The `configs` folder has a subdirectory for each platform.
* Under each subdirectory the configs for the service are placed.
* The service or utility name is used as the name for the config

Example
```
 configs
 ├── darwin
 │   ├── fan_service.json
 │   └── fw_util.json
 └── montblanc
     ├── platform_manager.json
     └── weutil.json
```

## Advantages
* Single place to hold all configs
* Config Lib takes care of basic validation of configs
* The configs can be validated at build time (instead of run time)
* We can write unit tests for config presence / absence
* The services code can be independent of platform

## Note
* The platform binaries should still be able to pass --config to
override fetching config from config_lib
