#!/bin/bash

if [[ $0 != '/bin/bash' ]]; then
  echo "Usage: . $0";
  exit 1
fi

_PWD=`pwd`
BIN=/hdd2/Espressif/sdk_3x/xtensa-lx106-elf/bin
cd $BIN

[[ ":$PATH:" != *":$BIN:"* ]] && PATH="${BIN}:${PATH}"

[ ! -e xt-xcc ] && ln -s xtensa-lx106-elf-gcc xt-xcc
[ ! -e xt-ar ] && ln -s xtensa-lx106-elf-ar xt-ar

cd $BIN
cd $_PWD
