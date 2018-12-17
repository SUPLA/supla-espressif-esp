#!/bin/sh

if [ ! -e /CProjects ]; 
then
   echo "/CProjects directory does not exist!"
   exit 1
fi

cd /CProjects

if [ -e ./supla-espressif-esp ]; 
then
  echo "supla-espressif-esp already cloned!"
  exit 1
fi

git clone https://github.com/SUPLA/supla-espressif-esp

exit 0
