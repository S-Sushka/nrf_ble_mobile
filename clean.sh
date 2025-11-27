#!/bin/bash

TOOLCHAIN_VARSION="v3.1.1"
SDK_VERSION="v3.1.1"
BUILD_DIR="./build"

if [[ -z "$NCS_PATH" ]]; then
  echo "Variable NCS_PATH must be specified. Default location is /opt/nordic/ncs"
  exit 1
else
  echo "===> Use NCS PATH: $NCS_PATH"
fi

# выбрать версию SDK которая будет использоваться
source $NCS_PATH/$SDK_VERSION/zephyr/zephyr-env.sh

nrfutil sdk-manager toolchain launch --ncs-version $TOOLCHAIN_VARSION west build -- -t clean --build-dir $BUILD_DIR
