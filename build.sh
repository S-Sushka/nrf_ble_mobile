#!/bin/bash

TOOLCHAIN_VARSION="v3.1.1"
SDK_VERSION="v3.1.1"
#TOOLCHAIN_VARSION="v3.0.2"
#SDK_VERSION="v3.0.1"
BOARD_MODEL="nrf52840dk/nrf52840"
BUILD_DIR="./build"

if [[ -z "$NCS_PATH" ]]; then
  echo "Variable NCS_PATH must be specified. Default location is /opt/nordic/ncs"
  exit 1
else
  echo "===> Use NCS PATH: $NCS_PATH"
fi

# удалить build если он есть
# rm -fr $BUILD_DIR

mkdir -p $BUILD_DIR

# выбрать версию SDK которая будет использоваться
source $NCS_PATH/$SDK_VERSION/zephyr/zephyr-env.sh
 
# запустить toolchain нужной версии, для сборки под модель чипа (борды)
nrfutil sdk-manager toolchain launch --ncs-version $TOOLCHAIN_VARSION west build -- --board=$BOARD_MODEL --build-dir $BUILD_DIR

# запустить toolchain нужной версии, для сборки под модель чипа (борды) с принудительной пересборкой всего проекта
#nrfutil sdk-manager toolchain launch --ncs-version $TOOLCHAIN_VARSION west build -- --board=$BOARD_MODEL --build-dir $BUILD_DIR --pristine