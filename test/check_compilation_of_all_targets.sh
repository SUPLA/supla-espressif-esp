#!/bin/bash

set -e

targets="
gate_module2_wroom
rs_module
sonoff
sonoff_ds18b20
sonoff_touch
sonoff_dual
sonoff_socket
sonoff_th10
sonoff_th16
sonoff_dual
h801
impulse_counter
"

failing_targets="
inCan_DS
inCan_DHT11
inCan_DHT22
inCanRS_DS
inCanRS_DHT11
inCanRS_DHT22
wifisocket
wifisocket_x4
wifisocket_54
gate_module
gate_module_dht11
gate_module_dht22
gate_module_wroom
EgyIOT
dimmer
rgbw_wroom
lightswitch_x2
lightswitch_x2_54
lightswitch_x2_DHT11
lightswitch_x2_54_DHT11
lightswitch_x2_DHT22
lightswitch_x2_54_DHT22
"

failed_targets=""

cd /CProjects/supla-espressif-esp/src

for i in ${targets}
do
  echo "======================================================"
  echo "Building target $i"
  echo "======================================================"
  echo
  ./build.sh $i
  if [ "$?" -ne "0" ]
  then
    echo "Building target $i failed"
    failed_targets="$failed_targets $i"
  fi

  echo "======================================================"

done

if [ "${#failed_targets}" -ne "0" ]
then
  echo "======================================================"
  echo "Failed targets:"
  echo "======================================================"
  echo $failed_targets
  echo "======================================================"
  exit 1
fi

echo "All targets compiled successfully!"

