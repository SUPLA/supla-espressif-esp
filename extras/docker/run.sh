#!/bin/sh

PROJ=~/CProjects
[ -e $PROJ ] || mkdir -p $PROJ
cd $PROJ
[ ! -e ./supla-espressif-esp ] && git clone https://github.com/SUPLA/supla-espressif-esp
docker run -v "$PROJ":/CProjects -it devel/esp8266 /bin/bash
