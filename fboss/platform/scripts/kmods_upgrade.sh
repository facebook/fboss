#!/bin/sh
# Copyright 2022-present Facebook. All Rights Reserved.

stop_chef(){
  # Stopping chef to perform kernel upgrades. Default is at least 1 hour

  /usr/facebook/ops/scripts/chef/stop_chef_temporarily -r "To perform BSP Kmods upgrade on Darwin platform"

}

stop_platform_services(){
  # stop rackmon
  systemctl stop rackmon.service

  # stop fan_service
  systemctl stop fan_service.service

  # stop sensor_service
  systemctl stop sensor_service.service

  # stop data_corral_service
  systemctl stop data_corral_service.service

}

fan_speed_increase(){
  # since fan_service is not running to dynamically control the fan speed
  # so  we will increase the fan_speed to 70% to prevent the switch from overheating

  bash set_fan_speed.sh 70

}

stop_forwarding_stack_services(){
  # stop QSFP service
  systemctl stop qsfp_service

  # stop agent
  systemctl stop wedge_agent

}

stop_watchdog(){
  # stop watchdog
  # watchdog timer will be stopped automatically when watchdog service
  # is stopped.
  systemctl stop watchdog

}

unload_bsp_kmods(){
  # unload the I2C client drivers
  modprobe -r rook_fan_cpld blackhawk_cpld i2c_dev_sysfs amax5970 aslg4f4527

  # unload SCD and SCD subdevice drivers
  modprobe -r scd_smbus scd_leds scd_watchdog scd
}


install_bsp_kmods(){
  # Remove the old RPM
  KMOD_RPM_VER="$1"
  old_rpm=$(rpm -qa | grep arista_darwin_bsp)
  rpm -e "$old_rpm"

  fbk_ver=$(uname -r)
  dnf install -y "arista_darwin_bsp-${fbk_ver}-${KMOD_RPM_VER}"
}

reload_bsp_kmods(){
  # loading scd driver should be good enough: all the relevant scd-* and
  # I2C client drivers should be loaded automatically when the corresponding
  # devices are created.
  modprobe scd
}

start_watchdog(){
  # restart the watchdog service
  systemctl start watchdog

}

start_forwarding_stack_services(){
  # start wedge_agent
  systemctl start wedge_agent

  # start qsfp service
  systemctl start qsfp_service

}

start_platform_services(){
  # start data_corral
  systemctl start data_corral_service.service

  # start sensor_service
  systemctl start sensor_service.service

  # start fan_service
  systemctl start fan_service.service

  # start rackmon
  systemctl start rackmon.service

}

# start chef
start_chef(){
  # resume chef operation
  rm -rf /var/chef/cron.default.override

}


if [ "$#" -eq 1 ]; then
  KMOD_RPM_VER="$1"

  # Drain the swich since Kernel upgrade is being treated as disruptive
  # TODO: Figure out what is the command to drain switch on Darwin

  # stop chef so that it doesn't restart platform services
  stop_chef

  # Stop platform services
  stop_platform_services

  # increase fan_speed
  fan_speed_increase

  # stop forwarding stack services
  stop_forwarding_stack_services

  # stop watchdog
  stop_watchdog

  # unload bsp kernel Modules
  unload_bsp_kmods

  # install new bsp kmods
  install_bsp_kmods "$KMOD_RPM_VER"

  # reload bsp_kmods
  reload_bsp_kmods

  # start watchdog service
  start_watchdog

  # start fowarding stack services
  start_forwarding_stack_services

  # start platform services
  start_platform_services

  # start chef
  start_chef

  # undrain the switch
  # TODO: Figure out the command to undrain the switch
else
  echo "Wrong usage"
  echo "proper usage: kmods-updade.sh <KMOD_RPM_VER>"
  echo "where KMOD_RPM_VER is the latest kmod rpm version"
fi
