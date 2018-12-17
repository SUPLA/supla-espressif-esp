#!/bin/sh

PROJ=~/CProjects
[ -e $PROJ ] || mkdir -p $PROJ
docker run -v "$PROJ":/CProjects -it devel/esp8266 /bin/bash
