# Fan Service - A FBOSS Platform Service

## Objective
1. The primary functionality of this service is to perform thermal control for complete FBOSS based switch Chassis
2. The service is also capable of detecting dead fan / sensor and behave differently in case of module failures.

## Design Concept
* Configuration driven, platform sepecific fan service configuration JSON files are located in config_lib:
1. Fan Service will define different "Zones", along with which sensors determine the fan speed (pwm) of the zone, and how it is calculated (Max of all PWM, Min or average)
2. A system can have only one zone or multiple zones. All of the above items are defined through a config file. That is, this service is config based.
3. Each sensor has its own table or formula to calculate the pwm value from the sensor reading. If a table based method is used, multiple tables will be defined so that we apply different conversion methods when system is in good state or when some module failed.

## Usage
* The fan service will auto start by systemd service when it's deployed in Meta DC.
* In order to start fan_service in OSS environment, simply run fan_service, and it will determine the system type and load the correct config.
* Some parameter that can be used :
1. thrift_port : this will specify which thrift port to use (default 5972)
2. control_interval : this will determine how often fan_service will check the system status and program pwm value as needed, in terms of seconds. The default value is 5.

## Trouble Shooting
* If the fan_service fails at start up due to thrift port conflict : This means the thrift port is already used. Try killing redundant process running, or start fan_service with different thrift port (check the Usage section)
* To track abnormal behavior :
1. Please check the log file of the fan_service located in /var/facebook/log/fan_service.log
2. The logfile will tell you which sensor reading determined the pwm of zone, and how the fan control attempts went.

## Note
* The run time perameter "--config" can be used to specify configuration JSON file
to override fetching config from config_lib
