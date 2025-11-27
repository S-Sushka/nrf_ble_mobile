#!/bin/bash

TOOLCHAIN_VARSION="v3.1.1"
SDK_VERSION="v3.1.1"
BUILD_DIR="./build"
BOARD_ID=1050211414

if [[ -z "$NCS_PATH" ]]; then
  echo "Variable NCS_PATH must be specified. Default location is /opt/nordic/ncs"
  exit 1
else
  echo "===> Use NCS PATH: $NCS_PATH"
fi

# выбрать версию SDK которая будет использоваться
source $NCS_PATH/$SDK_VERSION/zephyr/zephyr-env.sh

nrfutil sdk-manager toolchain launch --ncs-version $TOOLCHAIN_VARSION west flash -- -d $BUILD_DIR --dev-id $BOARD_ID
